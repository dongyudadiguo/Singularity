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

Important: after any tool round finishes, `ae.py` always continues to another
model call. It cannot "stop after compaction" by itself. Viewer launches it with
`AE_RUNNER=1` so the compaction child can terminate that parent process after
replacing the transcript.

## Active ae.py Run (default: compact and stop)

A child may compact the file directly because the parent is blocked until the
child exits. Default active compaction:

1. Replaces all non-system messages with one summary user message.
2. Stops the parent `ae.py` when `AE_RUNNER=1` so it does not auto-continue.

Pass the summary **inline as a string**. Do **not** write or read
`current_summary.md` (or any other summary sidecar file).

Run from a Python tool:

```python
from skills.context_compaction.compact import compact_active_file

summary = """...compacted engineering context..."""
print(compact_active_file("input.json", summary))
```

Do not expect the current tool round to continue after this returns. The parent
runner is intentionally ended. Start a new viewer run when work should resume
from the summary.

Legacy behavior that keeps the in-flight tool group and lets `ae.py` continue is
still available as `compact_active_file_keep_tools`, but do not use it unless the
user explicitly wants auto-continue after compaction.

## Offline Command

When no `ae.py` process is using the file, compact all non-system history with
an inline summary:

```bat
python -m skills.context_compaction.compact input.json --summary "...compacted engineering context..."
```

For an active run that should stop:

```bat
python -m skills.context_compaction.compact input.json --summary "...compacted engineering context..." --active
```

Legacy keep-tools active compact:

```bat
python -m skills.context_compaction.compact input.json --summary "...compacted engineering context..." --active-keep-tools
```

`--summary-file PATH` remains available only as an optional convenience when the
summary already exists elsewhere. Prefer `--summary` / the Python string argument.
Never create `skills/context_compaction/current_summary.md` as part of compaction.

A positive `--keep-from-user N` retains the latest `N` user turns and all
subsequent messages. `--keep-from-index INDEX` is for an exact, validated
offline boundary. Neither retention option may be combined with `--active` or
`--active-keep-tools`.

## Required Behavior

1. Read the current `ae.py` before choosing the compaction mechanism.
2. Do not print or manually inspect the complete `input.json` merely to compact it.
3. Preserve all leading `system` messages exactly.
4. Replace archived messages with one `user` summary message.
5. Build the summary in memory / as a Python string; do not generate a summary markdown file.
6. Default active compaction stops the runner; do not leave it free to make another model call.
7. Reject orphaned retained tool results and invalid boundaries.
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
