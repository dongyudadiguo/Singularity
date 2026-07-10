import argparse
import json
import os
import shutil
from datetime import datetime
from pathlib import Path

SUMMARY_PREFIX = "[Compacted context summary; archived messages were replaced locally]"
REQUEST_SUFFIX = ".compact-request.json"


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


def request_path(input_path):
    path = Path(input_path).resolve()
    return path.with_name(path.name + REQUEST_SUFFIX)


def request_compaction(input_path, summary, keep_user_turns=0):
    """Ask ae.py to compact after the current tool result is persisted."""
    if not summary.strip():
        raise ValueError("summary is empty")
    request = request_path(input_path)
    temp = request.with_name(request.name + ".tmp")
    payload = {
        "summary": summary,
        "keep_user_turns": keep_user_turns,
        "summary_role": "user",
    }
    temp.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    os.replace(temp, request)
    return str(request)


def apply_compaction_request(input_path):
    request = request_path(input_path)
    if not request.exists():
        return None
    payload = json.loads(request.read_text(encoding="utf-8"))
    result = compact_file(
        input_path,
        payload["summary"],
        keep_user_turns=payload.get("keep_user_turns", 0),
        summary_role=payload.get("summary_role", "user"),
    )
    request.unlink()
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compact ae.py message context")
    parser.add_argument("input_json")
    parser.add_argument("--summary-file", required=True)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--keep-from-index", type=int)
    group.add_argument("--keep-from-user", type=int, default=0)
    args = parser.parse_args(argv)
    summary = Path(args.summary_file).read_text(encoding="utf-8")
    result = compact_file(
        args.input_json,
        summary,
        keep_from_index=args.keep_from_index,
        keep_user_turns=args.keep_from_user,
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
