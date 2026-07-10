import argparse
import json
import os
import signal
import subprocess
import shutil
from datetime import datetime
from pathlib import Path

SUMMARY_PREFIX = "[Compacted context summary; archived messages were replaced locally]"


def _leading_system_count(messages):
    count = 0
    for message in messages:
        if message.get("role") != "system":
            break
        count += 1
    return count


def _boundary_from_user_turns(messages, keep_user_turns):
    if keep_user_turns < 0:
        raise ValueError("keep_user_turns must be non-negative")
    if keep_user_turns == 0:
        return len(messages)
    users = [i for i, message in enumerate(messages) if message.get("role") == "user"]
    if not users:
        return len(messages)
    return users[max(0, len(users) - keep_user_turns)]


def _validate_retained_tools(messages):
    calls = set()
    for message in messages:
        for call in message.get("tool_calls") or []:
            call_id = call.get("id")
            if call_id:
                calls.add(call_id)
        if message.get("role") == "tool":
            call_id = message.get("tool_call_id")
            if call_id not in calls:
                raise ValueError(
                    f"retained tool result {call_id!r} has no retained assistant tool call"
                )


def _active_tool_boundary(messages):
    """Return the assistant index for the tool group currently run by old ae.py."""
    for index in range(len(messages) - 1, -1, -1):
        message = messages[index]
        if message.get("role") != "assistant" or not message.get("tool_calls"):
            continue

        call_ids = {
            call.get("id") for call in message["tool_calls"] if call.get("id")
        }
        trailing = messages[index + 1 :]
        if all(
            item.get("role") == "tool"
            and item.get("tool_call_id") in call_ids
            for item in trailing
        ):
            return index

        # A later non-tool message means this group is already complete history.
        break
    raise ValueError("no active assistant tool-call group found")


def compact_file(
    input_path,
    summary,
    keep_from_index=None,
    keep_user_turns=0,
    summary_role="user",
):
    path = Path(input_path).resolve()
    data = json.loads(path.read_text(encoding="utf-8"))
    messages = data.get("json", {}).get("messages")
    if not isinstance(messages, list):
        raise ValueError("input JSON has no json.messages list")
    if not summary.strip():
        raise ValueError("summary is empty")

    system_count = _leading_system_count(messages)
    if keep_from_index is None:
        boundary = _boundary_from_user_turns(messages, keep_user_turns)
    else:
        boundary = keep_from_index
    if boundary < system_count or boundary > len(messages):
        raise ValueError(
            f"boundary {boundary} must be between {system_count} and {len(messages)}"
        )

    retained = messages[boundary:]
    _validate_retained_tools(retained)
    summary_message = {
        "role": summary_role,
        "content": SUMMARY_PREFIX + "\n\n" + summary.strip(),
    }
    compacted = messages[:system_count] + [summary_message] + retained

    before_messages = len(messages)
    before_bytes = path.stat().st_size
    data["json"]["messages"] = compacted
    encoded = json.dumps(data, ensure_ascii=False, indent=2).encode("utf-8")

    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    backup = path.with_name(path.name + f".precompact-{stamp}.bak")
    shutil.copy2(path, backup)
    temp = path.with_name(path.name + ".compact.tmp")
    try:
        with temp.open("wb") as handle:
            handle.write(encoded)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temp, path)
    finally:
        if temp.exists():
            temp.unlink()

    return {
        "backup": str(backup),
        "boundary": boundary,
        "messages_before": before_messages,
        "messages_after": len(compacted),
        "bytes_before": before_bytes,
        "bytes_after": len(encoded),
    }


def _stop_parent_runner():
    """Stop parent ae.py so it cannot start another API turn after compaction.

    Old ae.py always continues the while-loop once tool children return. The only
    way to honor "compact then stop" without editing ae.py is to end that parent
    process after the replacement is on disk. Tool children inherit AE_RUNNER=1
    when launched from viewer.py.
    """
    if os.environ.get("AE_RUNNER") != "1":
        return {"attempted": False, "reason": "AE_RUNNER not set"}

    ppid = os.getppid()
    if not ppid or ppid <= 1:
        return {"attempted": False, "reason": "no parent pid"}

    try:
        if os.name == "nt":
            completed = subprocess.run(
                ["taskkill", "/PID", str(ppid), "/F"],
                capture_output=True,
                text=True,
                errors="ignore",
            )
            return {
                "attempted": True,
                "pid": ppid,
                "ok": completed.returncode == 0,
                "returncode": completed.returncode,
                "output": ((completed.stdout or "") + (completed.stderr or "")).strip(),
            }
        os.kill(ppid, signal.SIGTERM)
        return {"attempted": True, "pid": ppid, "ok": True}
    except OSError as exc:
        return {"attempted": True, "pid": ppid, "ok": False, "error": str(exc)}


def compact_active_file(input_path, summary):
    """Replace non-system history with summary and stop the active ae.py runner.

    Retaining the in-flight tool group would leave the old runner free to make
    another model call after this child exits. For compaction-as-stop we write a
    clean summary-only transcript, then terminate the parent runner when this
    process was launched under AE_RUNNER=1.
    """
    result = compact_file(input_path, summary, keep_user_turns=0)
    result["stopped_parent"] = _stop_parent_runner()
    return result


def compact_active_file_keep_tools(input_path, summary):
    """Legacy active compact: keep the current tool group and do not stop ae.py."""
    path = Path(input_path).resolve()
    data = json.loads(path.read_text(encoding="utf-8"))
    messages = data.get("json", {}).get("messages")
    if not isinstance(messages, list):
        raise ValueError("input JSON has no json.messages list")
    boundary = _active_tool_boundary(messages)
    return compact_file(path, summary, keep_from_index=boundary)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compact ae.py message context")
    parser.add_argument("input_json")
    parser.add_argument("--summary-file", required=True)
    parser.add_argument(
        "--active",
        action="store_true",
        help="active compact-and-stop (summary only; stops parent when AE_RUNNER=1)",
    )
    parser.add_argument(
        "--active-keep-tools",
        action="store_true",
        help="legacy active compact that retains the current tool-call group",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--keep-from-index", type=int)
    group.add_argument("--keep-from-user", type=int, default=0)
    args = parser.parse_args(argv)
    summary = Path(args.summary_file).read_text(encoding="utf-8")
    if args.active and args.active_keep_tools:
        parser.error("--active cannot be combined with --active-keep-tools")
    if args.active:
        if args.keep_from_index is not None or args.keep_from_user:
            parser.error("--active cannot be combined with a retention boundary")
        result = compact_active_file(args.input_json, summary)
    elif args.active_keep_tools:
        if args.keep_from_index is not None or args.keep_from_user:
            parser.error("--active-keep-tools cannot be combined with a retention boundary")
        result = compact_active_file_keep_tools(args.input_json, summary)
    else:
        result = compact_file(
            args.input_json,
            summary,
            keep_from_index=args.keep_from_index,
            keep_user_turns=args.keep_from_user,
        )
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
