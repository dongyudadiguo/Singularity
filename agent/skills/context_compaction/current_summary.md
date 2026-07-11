# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
- Server source mirror (for network explorer / protocol): `C:\Users\12159\Desktop\server` (`server.go`).
- Preserve registered 32-byte `id.bin` identity beginning `5673fae3`; never replace with rejected identity beginning `66ee6f28`.
- Worktree is dirty. `agent/input.json` is runner-managed; do not revert unrelated changes.
- Do not modify `agent/ae.py` unless the user explicitly asks.
- User tests `vm.exe` manually; do not auto-launch it from agent (console popups interfere). Prefer short stability probes only when debugging crashes.
- Answer the user in Chinese when they ask `用中文回答` or prefer Chinese.

## Architecture truths
- Fixed bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`.
- VM block format: `token[32] + payload_size[u32 LE] + payload`, ending with zero token.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild.
- Modularization rule (user, hard): **if a mod can be composed from lower-level tokens, delete/replace it; mods must stay as low-level as possible.** Prefer bare logical tokens over specialized natives/wrappers. Prefer stream-embedded compositions over integrated DLLs.
- Do not use `cvm_exec_payload` from frame/program conditionals: it rewrites live payload key->content hash and corrupts reexecing program cache / can async-write a bad override. `cond_payload` must call non-mutating `cvm_exec(token)`.
- `var_set_payload` trap: payload total size exactly 36 with legacy layout means ALLOCATE; new layout is `id_len[u32]+id+size[u32]` (or +data).
- First boot must stay small general-purpose atomic mods. Never restore integrated `ui_*`, `uistate`, `editor_frame`, or `editor_init`.
- Decomposability: specialized/integrated tokens need firstchild token blocks / surfaces.

## Variable IDs (current)
- **Prefer plaintext UTF-8 string ids**, not sha256 digests. Examples:
  - `atomic.editor.input`, `atomic.cam.x/y/z`, `atomic.views.table`
  - `atomic.cam.grab_*`, `atomic.cam.pan_active`, `atomic.cam.drag_anchored`
  - `atomic.input.prev_lmb`, `atomic.input.prev_rmb`
- vmvar: id is arbitrary binary `id_len` ≤ 256.
- Encoding:
  - `var_read_payload`: entire payload = id
  - `var_write_payload` (stack write): **must** be `id_len[u32]+id` for non-32 ids
  - `var_set_payload`: `id_len+id+size` or `id_len+id+data`
  - `views_*` payloads: **`id_len[u32]+id+args`** (legacy fixed-32 still accepted)
- Editor var-row display: specialized layout for `var_*` (icon + id + size); printable id shows as string, else hex.
- **Bug fixed:** `mouse_edge` previously wrote prev_* with bare string id (failed legacy-32 path) → RMB edge every frame → drag hitch. Now uses `u32(len(prev))+prev`.

## Current first-boot composition
- Generator: `generate_atomic_first_boot.py`; installer: `install_generic_first_boot.py`.
- Thin program orchestrator (bare only):  
  `frame_begin → ensure → camera → frame_clear → input → zoom → camera → render_draw → hud → editor → frame_end → reexec`
- Modules (5): `camera`, `input`, `zoom`, `hud`, `editor`.
- Surfaces:
  - `views` (anchor `views_ensure`): ensure, pick, apply_lmb, apply_rmb, open_key, select_row, set_active_drag, active_key/cursor, set_cursor_active, drag_end, get/set_drag_xy
  - `views_paint` (anchor `views_paint_rows`): paint_links, paint_titles, paint_rows, render_draw
- `render_draw` composition: `views_paint_links → views_paint_titles → views_paint_rows`
- Actions (~13): delete, insert, backspace, clear, click, rmb, pan, drag, begin_*, end_*, zoom_apply
- Zoom: `z' = clamp(z * (1 + notches*0.10), 0.15..6)`, mouse-centered; wheel via `g_wheel_latched` in `dxgfx`.
- Cursor add/dec: **recipes** (not specialized natives): active_cursor ±1, clamp with `block_resolve`+`block_instr_count`+`i32_min/max`, then `views_set_cursor_active`.
- String edits: stack `string_append`/`string_backspace` + var read/write; clear = zeros + write. No `string_*_var` in first-boot.
- Names: `name_prefix_find` / `name_lookup` (not `registry_*` in first-boot).
- Input edge: `mouse_button_down` + prev var + not/and (no `mouse_button_pressed` in first-boot).
- Hash carriers for open (by instruction **name**, not hardcoded DLL hash): `cond_payload`, `jump_payload`, `exec_payload`, `cond_reexec` (+ related). RMB open uses payload 32-byte target when carrier.

## Pointer / drag behavior (CURRENT — about to change)
- **Current:** LMB = select (`views_apply_lmb`); RMB on title = start drag; RMB on row = open/drag-out linked view (`views_apply_rmb`); MMB (button 4) = pan.
- Drag while RMB held uses `drag` action + `drag_xy` + grab anchors; end on RMB up.
- **USER NEXT INTENT (do this next):**
  1. **LMB drag title** (move node) — not RMB title drag.
  2. **RMB** = **close/remove node** (not drag title).
  3. **RMB drag-out from row** still desired as open-child? User said: 右键拖出 remains for drag-out; 右键关闭节点. Clarify in implementation: title RMB → close; row RMB-drag → open/drag-out if still wanted, or redefine consistently.
  4. Bottom-left input completion should show **path** of the matched token (where it lives in graph/name tree), not only the name.
  5. **New network token explorer:** same interaction model as editor views; default list starts at `#TAG`; rows are RMB-draggable out (into editor/world). Server mirror at `C:\Users\12159\Desktop\server`.

## Critical performance / crash notes
- Stale `cond_payload` must import **`cvm_exec` only** (not `cvm_exec_payload`).
- **Do not uget/override on render hot path** (corrupts main `conn` / instruction stream).
- Token color flags: cyan=DLL (`cvm_has_dll`), amber=local block cache, magenta=both, gray=neither. Restored in `views_paint_rows`. If using `cvm_cache_hit` during paint, **re-resolve view key** afterward — `cvm_cache_hit` mutates `primary_idx`.
- Resolve hitch (~1s) when switching pan↔rmb: block cache was 32 slots + cold `uget` on main conn. **vmstore now `CACHE_SLOTS 256`**; resolve prefers override cache (incl. negatives) + `cvm_children` disk cache before network. Rebuild/load new `vmstore.dll` (user must restart vm.exe).
- Icons: no hollow rect for unknown tokens.

## Modularization status (~9/10 mid-low)
- Removed from first-boot: mega `views` op-table, `views_render`, `registry_*`, `string_*_var`, `mouse_button_pressed`, `views_pointer_*`, `views_cursor_*`.
- Remaining domain mid-low natives: `views_apply_*`, `views_paint_rows`, thin table field ops, platform gfx/input, `cond_payload`, `name_*`.
- Disk leftovers may still exist (`views.c`, `views_render.c`, `registry_*.c`, old pointer mods) — not first-boot.

## Important files
- `generate_atomic_first_boot.py`, `install_generic_first_boot.py`
- `atomic_module_blocks/`, `atomic_action_blocks/`, `atomic_surface_blocks/`
- `atomic_first_boot_manifest.json`, `atomic_mod_tokens.txt`, `instruction_names.bin`, `mod_tokens.txt`
- `mods_src/views_common.h`, `views_apply_*.c`, `views_paint_*.c`, `views_*.c`, `name_*.c`, `string_append.c`, `string_backspace.c`
- `vmstore.c` (cache 256, resolve path), `vm.c` (`cvm_has_dll`), `vmvar.c` (arbitrary id)
- `dxgfx.cpp` / `dxgfx.dll` (wheel latch, icons, world mouse)
- `C:\Users\12159\Desktop\server\server.go` (protocol / #TAG / children)

## Verification / ops
- After native hash changes: rebuild DLL → hash-rename into `mods/<sha>.dll` → update `mod_tokens.txt` / `atomic_mod_tokens.txt` → `python generate_atomic_first_boot.py` → `python install_generic_first_boot.py`.
- User must restart `vm.exe` to pick up `vmstore.dll` / mods / program override.
- Do not auto-launch `vm.exe` from agent.

## Exact next tasks (user intent at compact time)
User asked to compact context, then in the new context implement:

1. **Pointer remap:** LMB drag title to move nodes; RMB closes node; keep sensible row open/drag-out behavior (confirm while implementing).
2. **Completion path:** bottom-left match/completion UI shows the completed token’s **path** (graph/tag path), not only name.
3. **Network token explorer:** new view/list using same editor interaction language; default root = children of `#TAG` (see server); support RMB drag-out like editor rows.
4. Use server mirror `C:\Users\12159\Desktop\server` for protocol/ops details.

Do not re-litigate finished modularization unless needed for these features.
