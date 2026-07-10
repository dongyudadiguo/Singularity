# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
- Preserve registered 32-byte `id.bin` identity beginning `5673fae3`; never replace with rejected identity beginning `66ee6f28`.
- Worktree is dirty. `agent/input.json` is runner-managed; do not revert unrelated changes. Do not modify `agent/ae.py` unless the user explicitly asks.
- VM block format: `token[32] + payload_size[u32 LE] + payload`, ending with zero token.
- Native mod DLL names are SHA-256 content hashes. Logical keys resolve by user override then graph child.
- First boot must be small general-purpose atomic mods. Never restore integrated `ui_*`, `uistate`, `editor_frame`, or `editor_init`.

## Architecture truths
- Fixed bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`.
- `generate_atomic_first_boot.py` builds `first_block.bin`, `first_program_block.bin`, action blocks, `atomic_first_boot_manifest.json`, and local `instruction_names.bin`.
- `install_generic_first_boot.py` validates declared hash-named natives, uploads DLLs/blocks, sets overrides, votes bootstrap child.
- `atomic_mod_tokens.txt` maps source names to current DLL tokens for newly compiled atomic mods.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild. Platform decides.
- Distinguish: execute a callable token (native or instruction block) vs open/select/edit a non-instruction object token.
- Prefer static structure by embedding block tokens in parent streams. Use `cond` for key-triggered actions. Use `exec`/`exec_payload` only when needed for function-pointer style targets or pinned content hashes; do not use `exec` as default modularization.
- `cvm_exec`: native hit runs DLL; miss enters block. `jump_payload` replaces current stream (no return). `reexec` restarts current block without leaking frames.
- `var_set_payload` trap: payload `id[32] + u32` with total size exactly 36 means ALLOCATE that many zero bytes, not write 4 data bytes. For 4-byte initials: allocate, then `const_payload` + `var_write_payload`.

## Reference UIs
- Git commit `2385b23` 「终于重置好了」: `ui_init/registry/input/edit/render` + `uistate` multi-view editor (names, completion, insert/delete/move, Alt child, OEM_3 data, MMB pan, wheel zoom, right-click linked views, drag views, hit-test). Reference for target behavior, not to restore as integrated DLLs.
- `C:\Users\12159\Desktop\transition\main.c` `to_dest_dev_base`: older richer SelfEdit (structure colors, brackets, selection copy/paste, mouse-follow HUD). Larger than `2385b23`; not phase-1 target.

## Current deployed first boot
- Latest first hash: `dfbe0eebd28d8715ec8e0ad432478523d3e662858ae8535aed050c9a8b9949c7`.
- Latest program hash: `67a2058c89c295e4057ac5236c3f61a322becf6810e3b1f6285ef7c6594f32f5` (~348 instructions).
- Actions: `down/up/delete/insert/backspace/clear/save` plus `pan`, `click`, `set_cursor`.
- Camera vars: `atomic.cam.x/y/z`, `atomic.cam.last_mx/my`; hit var `atomic.cursor.last_hit`; editor input/cursor vars retained.
- Frame behavior: `camera_set_stack` from vars; MMB pan via `mouse_button_down(4)+cond->pan`; wheel zoom every frame (`zoom *= 1+0.1*wheel`, clamp 0.15..6); `world_mouse`+`hit_row` stores relative row; LMB click if `last_hit>=0` sets `cursor += last_hit`; list draw + screen-space HUD; keyboard actions unchanged.
- Verified: VM stable ~32MB (after fixing var alloc trap that previously ballooned to multi-GB and AV'd). Dark UI renders. Down/Up list move works. Click select changes list. MMB pan changes view. Wheel zoom later verified with mouse_event/PostMessage after focus-sensitive first attempt failed.
- `mouse_x`/`mouse_y` rewritten to self-call `dxgfx_mouse` (old versions only popped a 16-byte mouse state and would underflow if used alone).
- `dxgfx`: wheel cleared in `frame_end` so same-frame peeks see consistent delta.

## New/updated atomic mods (not exhaustive)
- Control: `exec`, `exec_payload` (unconditional stack/payload exec; dual-identity).
- Input/camera/geometry: `world_mouse`, `mouse_wheel`, `mouse_button_down`, `mouse_button_pressed` (per-button edge), self-contained `mouse_x`/`mouse_y`, `camera_set_stack`.
- Float/stack: `f32_const`, `f32_add/sub/mul/div`, `f32_clamp`, `f32_neg`, `i32_to_f32`, `dup_u32`, `drop_u32`, `swap_u32`, `hit_row`, `point_in_rect`, `i32_max`.
- Tokens recorded in `atomic_mod_tokens.txt`; `build_mods.bat` patched for rebuilds.

## Agent tooling notes
- User asked not to change `ae.py`. Terminal flash on tool calls comes from `python -c` children using `sys.executable`.
- Fix without touching `ae.py`: `agent/viewer.py` starts agent via `pythonw.exe` (`agent_python()` + optional `CREATE_NO_WINDOW`). Restart agent from viewer for effect. `view.vbs` already launches viewer with `pythonw`.
- Compaction: use `skills.context_compaction.compact.compact_active_file` while ae is blocked on tools.

## Still missing vs 「终于重置好了」
- Multi-view table, right-click open linked view, view dragging, parent-child lines.
- Alt create child block + new view; OEM_3/data payload insert.
- Stronger x-range hit testing on list rows; selection highlight still mostly ">" / fixed rect, not full per-row active styling for arbitrary cursor.
- Do not implement as integrated UI DLL; continue atomic natives + logical blocks, prefer stream-embedded blocks over `exec`.

## Exact next task
- Continue restoring spatial multi-view editor features as small atomic ops + ordinary logical blocks.
- Recommended order: multi-view state/draw/drag/link lines → right-click open chain → Alt child / data insert.
- Keep keyboard editor regression green; reinstall via generate+install after program changes; restart `vm.exe` after DLL token changes.
