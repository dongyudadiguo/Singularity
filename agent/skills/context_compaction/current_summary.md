# Active Project Context

## Workspace

- Current project: `C:\Users\12159\Desktop\Singularity`
- Server source mirror: `C:\Users\12159\Desktop\server\server.go`
- Legacy UI reference: `C:\Users\12159\Desktop\transition\main.c`
- Live server: `118.25.42.70:9000`
- `id.bin` currently contains the live-server registered 32-byte identity beginning `5673fae3`; do not replace it with the rejected identity beginning `66ee6f28`.
- Worktree is dirty. Never revert unrelated/user changes, especially `agent/input.json` changes produced by the Agent runner.

## Architecture And Constraints

- VM block format: repeated `token[32] + payload_size[u32 LE] + payload`, terminated by a 32-byte zero token.
- Native mod DLL filenames are their SHA-256 hashes. Logical block keys resolve through user override first, then graph first child.
- First boot must be composed from general-purpose mods and editable logical blocks. Do not restore the removed giant `editor_frame`/`editor_init` integrated mod.
- Instruction registry is traversed from `sha256("#TAG")`. Tag file contents begin with `#`; instruction token children have a child file containing their display name.
- Shared `uistate.dll` is data-only. Behavior is split across `ui_init`, `ui_registry`, `ui_input`, `ui_edit`, and `ui_render` mods.

## Current Modular First Boot

- `first_block.bin` initializes UI state, scans registry, and executes logical program key `ad091072db93a94653168dd35921e72bd97de8a3e7cff503afbbb085cd847f6f`.
- `first_program_block.bin` composes `frame_begin -> frame_clear -> ui_input -> ui_edit -> ui_render -> frame_end -> reexec`.
- Bootstrap native token remains `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- `install_generic_first_boot.py` installs the modular blocks, sets required user overrides, votes the bootstrap child, uploads UI DLLs, and registers them under `#ui`.
- Latest deployed program block after exact hit-testing and visible hover feedback: `1d00407efe80f95e813adde37e1aac4da65205f9d8c2c13692004c8df9f7d71f`.
- Latest `ui_render` token: `9504b0e8f3610d7b95a24905ee811c3526e3c682f542d4ea8c0a02fbbe2cfb34`.

## Implemented UI Behavior

- Dark rendered window, token list, payload summaries, multiple linked views, text input/completion, selection, insert/delete, child block creation, camera pan/zoom, view dragging, and `Ctrl+S` flush.
- `dxgfx_measure_text()` uses DirectWrite text layout metrics with the same Consolas font/size as rendering.
- Token, title, and `<end>` hit regions use measured text width, not fixed row width.
- Token hover now has visible exact-width background feedback. Verified: right-clicking same-row blank space caused zero screenshot change; right-clicking token text opened a view. Hover entering/leaving token changed 975 screenshot bytes.
- A process must be fully restarted to load a newly hashed DLL.

## Recent Files Added Or Changed

- `uistate.c`, `uistate.h`, `uistate.dll`, `libuistate.a`
- `mods_src/ui_init.c`, `ui_registry.c`, `ui_input.c`, `ui_edit.c`, `ui_render.c`
- `build_uistate.bat`, `build_mods.bat`
- `dxgfx.cpp`, `dxgfx.h`, `dxgfx.dll`
- `first_block.bin`, `first_program_block.bin`
- `install_generic_first_boot.py`
- Giant integrated editor source/DLL versions and associated first-full-editor artifacts were deleted.

## Context Compaction Mechanism

- `agent/ae.py` sends the complete `json.messages` array on every API request.
- For tools, the parent first persists the assistant tool call, runs a child Python process, reloads `input.json`, appends the tool result, and then loops.
- Direct compaction from the child is incorrect because the parent appends the current tool result afterward and subsequent work logs accumulate again.
- `agent/skills/context_compaction/compact.py` now implements a deferred sidecar request. The parent consumes it only after a tool result is durable, and also checks for a pending request before the first API call on startup.
- Compaction now defaults to preserving zero non-system messages and inserts one `user` summary. This removes the current inspection/tool round while giving the next assistant an actionable input.
- Offline `python ae.py input.json --compact ...` remains available only when no agent loop is using the file.
- Synthetic lifecycle test passed: a complete tool group was reduced to exactly `system + user summary`; request cleanup, backup, atomic write, and syntax compilation passed.
- This summary omits credentials, raw tool logs, repeated status messages, and obsolete implementation details.

## Immediate Task

Continue from this summary. The context-compaction skill has been corrected; verify the pending request is consumed on the next Agent startup before doing unrelated work.
