# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
- Preserve registered 32-byte `id.bin` identity beginning `5673fae3`; never replace with rejected identity beginning `66ee6f28`.
- Worktree is dirty. `agent/input.json` is runner-managed; do not revert unrelated changes.
- Do not modify `agent/ae.py` unless the user explicitly asks.
- VM block format: `token[32] + payload_size[u32 LE] + payload`, ending with zero token.
- Native mod DLL names are SHA-256 content hashes. Logical keys resolve by user override then graph child.
- First boot must stay small general-purpose atomic mods. Never restore integrated `ui_*`, `uistate`, `editor_frame`, or `editor_init`.

## Architecture truths
- Fixed bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`.
- `generate_atomic_first_boot.py` builds first/program/action blocks + `atomic_first_boot_manifest.json` + local `instruction_names.bin`.
- `install_generic_first_boot.py` validates hash-named natives, uploads DLLs/blocks, sets overrides, votes bootstrap child.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild.
- Prefer stream-embedded blocks over `exec` for modularization. Use `cond` for key-triggered actions.
- `var_set_payload` trap: payload `id[32] + u32` with total size exactly 36 means ALLOCATE that many zero bytes, not write 4 data bytes.

## Current deployed first boot
- first_hash: `64e7f569e94ffe3f029298aecde99a35496f7802ff735c0cfc5cdc757d8a4549`
- program_hash: `ebb875a596030676e99b94dbf4c524f93cd649ecaa6721563202b23b6bb1e7a8` (~85 instructions)
- Actions: `down/up/delete/insert/backspace/clear/save` plus `pan/click/rmb/drag/end_drag`
- Multi-view state var: `atomic.views.table`
- Camera vars: `atomic.cam.x/y/z`, `atomic.cam.last_mx/my`
- Frame behavior: ensure/seed view0 if empty; camera from vars; MMB pan; RMB drag while held and end_drag on RMB up; wheel zoom clamp 0.15..6; LMB select via views op29; RMB open/drag via views op30; `views_render` draws all views + link lines; keyboard editor actions operate on active view key/cursor.
- Installer min instruction check lowered to 40 (multi-view frame is denser natives, fewer stream ops).
- User asked not to auto-launch `vm.exe` from agent because console popups still interfere; user tests VM manually.

## New/updated atomic mods for multi-view
- `views` multi-op table native over one var blob (init/ensure/count/active/get/set/drag/hit/pointer_lmb/pointer_rmb/open helpers).
- `views_render` draws titles, rows, selection, parent-child lines, payload summaries.
- Helpers: `block_select_stack`, `drawline_stack`, `drawrect_stack`, `drawtext_xy_stack`, `block_token_at_index`, `drop_bytes`, `views_open_at_row`.
- Current tokens of note:
- block_select_stack=c561f96338e3489e26e8e37a418922af6c9ef5e560aa6aa113231c81810f11d3
- views=55e68aa2afd23162fd5aea76981d912a8dfd6f664ba0a59633c7aceed8431232
- views_open_at_row=0c34e0988649dcdd1c66a3cfc1de51ee592ed87fa7bef6e2af3bb84e44f9dc3c
- views_render=56534d338b10e311e933c5a94d986021982203b78d840741706ea6d040a87231
- `build_mods.bat` patched to compile these.

## Agent tooling / compaction stop fix
- User constraint: do not edit `ae.py`.
- Root cause of "compact then auto-continue": old `ae.py` always does another API turn after tool results.
- Fix without editing `ae.py`:
  - `viewer.py` starts agent with `AE_RUNNER=1` and prefers `pythonw.exe` / `CREATE_NO_WINDOW`.
  - `skills.context_compaction.compact.compact_active_file` now writes summary-only non-system history and, when `AE_RUNNER=1`, terminates parent runner so it cannot auto-continue.
  - Legacy keep-tools continue path remains as `compact_active_file_keep_tools` / `--active-keep-tools`.
- Terminal flash may still appear for some tool children; user said they will test and revisit viewer later.

## Still missing vs reference multi-view editor (`2385b23`)
- Alt create child block + new view
- OEM_3/data payload insert
- Richer status HUD / completion polish
- Do not reintroduce integrated UI DLLs

## Exact next task
- Wait for user manual VM verification of multi-view (select, RMB open/drag, pan/zoom, keyboard edit).
- Then implement Alt-child and OEM_3 data insert as atomic ops + logical action blocks.
- Keep keyboard editor regression green; regenerate+install after program changes; restart `vm.exe` only when user requests or after they confirm popup issue is handled.
