# Context Compaction Skill

Use this skill when the user asks to compress, compact, summarize, or clear the current API context while retaining enough state to continue work.

## Understand ae.py First

`ae.py` sends the complete `json.messages` array on every API request. For a tool call it follows this sequence:

1. Append the assistant message containing `tool_calls`.
2. Launch the Python tool as a child process.
3. Reload `input.json` after the child exits.
4. Append the tool result.
5. Send the entire resulting message array again.

Therefore a child tool must not directly compact `input.json`. The parent would append the current tool result afterward, leaving inspection commands and output in the supposedly compacted context. Compaction during an active run must be deferred until step 4 is complete.

## Active ae.py Run

Create a deferred request from the Python tool:

```python
from pathlib import Path
from skills.context_compaction.compact import request_compaction

summary = Path("skills/context_compaction/current_summary.md").read_text(encoding="utf-8")
print(request_compaction("input.json", summary))
```

The parent `ae.py` checks for this sidecar request immediately after writing each tool result. It then replaces every completed non-system message with one `user` summary message. Using the `user` role matters: the summary is the new input the assistant must act on during the next API request.

The default `keep_user_turns=0` is intentional. Keeping the current user turn also keeps every assistant tool call and tool result after it, which is exactly the context that usually needs removing. Include all active requirements in the summary instead.

## Offline Command

Only when no `ae.py` process is using the file:

```bat
python ae.py input.json --compact --summary-file skills\context_compaction\current_summary.md --keep-from-user 0
```

A positive `--keep-from-user N` retains the latest `N` user turns and all subsequent messages. `--keep-from-index INDEX` is for an exact, already-validated boundary.

## Required Behavior

1. Read `ae.py` before changing the compaction mechanism.
2. Do not print or manually inspect the complete `input.json` merely to compact it.
3. Preserve all leading `system` messages exactly.
4. Replace archived messages with one `user` summary message.
5. During an active run, request compaction and let the parent apply it after persisting the tool result.
6. Default to retaining zero old user turns; the summary carries current requirements.
7. If retaining messages, preserve complete assistant tool-call/tool-result groups and reject orphaned results.
8. Create a timestamped backup and write atomically.
9. Report message and byte counts before and after compaction.
10. Never include credentials, raw tool logs, repeated status messages, or obsolete implementation detail in the summary.

## Summary Contents

Retain only high-signal continuation state:

- active user goal and explicit constraints;
- workspace and relevant file locations;
- architecture and stable data formats;
- completed changes and identifiers still in use;
- verification evidence and known defects;
- dirty-worktree cautions;
- the exact next task.
