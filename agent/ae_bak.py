import argparse
import json
import subprocess
from pathlib import Path
from sys import executable

import requests


def apply_pending_compaction(path):
    from skills.context_compaction.compact import apply_compaction_request

    result = apply_compaction_request(path)
    if result:
        print(json.dumps({"context_compaction": result}, ensure_ascii=False))


def run_agent(input_path):
    path = Path(input_path)
    apply_pending_compaction(path)
    while True:
        data = json.loads(path.read_text(encoding="utf-8"))
        body = data["json"]
        response = requests.post(
            data["url"], headers=data["headers"], json=body, timeout=300
        )
        response.raise_for_status()
        payload = response.json()
        message = payload["choices"][0]["message"]
        body["messages"].append(message)
        path.write_text(
            json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        if not message.get("tool_calls"):
            break
        for call in message["tool_calls"]:
            arguments = json.loads(call["function"]["arguments"])
            stdout = subprocess.run(
                [executable, "-c", arguments["code"]],
                text=True,
                errors="ignore",
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            ).stdout
            data = json.loads(path.read_text(encoding="utf-8"))
            data["json"]["messages"].append(
                {
                    "role": "tool",
                    "tool_call_id": call["id"],
                    "content": stdout,
                }
            )
            path.write_text(
                json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
            )
        # Apply only after every result from this assistant message is durable.
        # Applying inside the loop could orphan later results in a parallel group.
        apply_pending_compaction(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_json")
    parser.add_argument("--compact", action="store_true")
    parser.add_argument("--summary-file")
    boundary = parser.add_mutually_exclusive_group()
    boundary.add_argument("--keep-from-index", type=int)
    boundary.add_argument("--keep-from-user", type=int, default=0)
    args = parser.parse_args()

    if args.compact:
        if not args.summary_file:
            parser.error("--compact requires --summary-file")
        from skills.context_compaction.compact import compact_file

        summary = Path(args.summary_file).read_text(encoding="utf-8")
        result = compact_file(
            args.input_json,
            summary,
            keep_from_index=args.keep_from_index,
            keep_user_turns=args.keep_from_user,
        )
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return

    run_agent(args.input_json)


if __name__ == "__main__":
    main()
