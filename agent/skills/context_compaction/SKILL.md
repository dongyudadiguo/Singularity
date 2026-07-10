# Context Compaction Skill

Use this skill when the user asks to compress, compact, summarize, or clear the
current API context while retaining enough state to continue work.

## Understand ae.py First

The restored, old `ae.py` sends the complete `json.messages` array on every API
request. For a tool call it follows this sequence:

1. Append the assistant message containing all `tool_calls`.
2. Run each Python child synchronously.
3. After each child exits, reload `input.json` and append that tool result.
4. Send the complete message array again after all results are present.

This runner has no command-line compaction mode and does not consume deferred
sidecar requests. Do not use `ae.py --compact` or create a pending request.

## Active ae.py Run

A child may compact the file directly because the parent is blocked until the
child exits and reloads the file before appending its result. Active compaction
must retain the entire current assistant `tool_calls` message and any results
already appended for that group. This keeps all tool IDs valid, including when
the assistant emitted multiple calls.

Run from a Python tool:

```python
from pathlib import Path
from skills.context_compaction.compact import compact_active_file

summary = Path("skills/context_compaction/current_summary.md").read_text(encoding="utf-8")
print(compact_active_file("input.json", summary))
```

The current tool round remains after the summary. That is required by the API
protocol; it can be archived by a later compaction. Never directly call
`compact_file(..., keep_user_turns=0)` from an active child because the parent
would append an orphaned tool result.

## Offline Command

When no `ae.py` process is using the file, compact all non-system history with:

```bat
python -m skills.context_compaction.compact input.json --summary-file skills\context_compaction\current_summary.md
```

For an active run, the equivalent CLI is:

```bat
python -m skills.context_compaction.compact input.json --summary-file skills\context_compaction\current_summary.md --active
```

A positive `--keep-from-user N` retains the latest `N` user turns and all
subsequent messages. `--keep-from-index INDEX` is for an exact, validated
offline boundary. Neither retention option may be combined with `--active`.

## Required Behavior

1. Read the current `ae.py` before choosing the compaction mechanism.
2. Do not print or manually inspect the complete `input.json` merely to compact it.
3. Preserve all leading `system` messages exactly.
4. Replace archived messages with one `user` summary message.
5. During an active old-runner call, retain the complete current tool-call group.
6. Reject orphaned retained tool results and invalid boundaries.
7. Create a timestamped backup and write atomically.
8. Report message and byte counts before and after compaction.
9. Never include credentials, raw tool logs, repeated status messages, or obsolete implementation detail in the summary.

## Summary Contents

Retain only high-signal continuation state:

- active user goal and explicit constraints;
- workspace and relevant file locations;
- architecture and stable data formats;
- completed changes and identifiers still in use;
- verification evidence and known defects;
- dirty-worktree cautions;
- the exact next task.
