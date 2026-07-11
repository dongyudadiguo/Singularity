# Compacted Engineering Context

## Workspace
- Project: `C:\Users\12159\Desktop\Singularity`
- Live server: `118.25.42.70:9000`
- Server source mirror: `C:\Users\12159\Desktop\server`
- Identity: preserve registered 32-byte `id.bin` beginning `5673fae3` (never replace with rejected `66ee6f28`)
- Worktree dirty; `agent/input.json` runner-managed; do not edit `agent/ae.py` unless asked
- User runs `vm.exe` manually (do not auto-launch; console popups interfere)
- Answer in Chinese when user prefers Chinese / asks `用中文回答`

## Architecture truths
- Bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`
- VM block: `token[32] + payload_size[u32 LE] + payload`, zero token ends block
- Token dual identity: may be native DLL if present, else logical via resolve/override/firstchild
- Modularization rule (hard): if composable from lower tokens, delete specialized; stay low-level
- Do not use `cvm_exec_payload` from cond paths (mutates live payload / corrupts reexec cache)
- `var_set_payload`: new layout `id_len[u32]+id+size|data`; legacy fixed-32 still works
- First boot: thin orchestrator + bare logical modules; no integrated ui_*/editor_frame
- POLICY: compose > specialized native/alias

## Variable IDs (current)
- Prefer **plaintext string ids** (not sha256 digests), e.g. `atomic.cam.x`, `atomic.editor.input`, `atomic.views.table`
- `var_read_payload`: entire payload = id
- `var_write_payload` stack-write: must use `id_len+id` (not bare string as legacy-32)
- `views_*` payload: `id_len[u32]+id+op_args` (legacy first-32 still accepted)
- Editor var-row paint: icon+id+size colors; printable id shows as text

## First-boot composition (current)
- Generator: `generate_atomic_first_boot.py`; installer: `install_generic_first_boot.py`
- Orchestrator: `frame_begin → ensure → camera → frame_clear → input → zoom → camera → render_draw → hud → editor → frame_end → reexec`
- Modules: camera, input, zoom, hud, editor
- Actions include: delete, insert, backspace, clear, click, rmb, pan, drag, begin_*, end_*, zoom_apply
- Zoom: clamp z*(1+notches*0.10) mouse-centered; wheel latched in dxgfx
- Multi-view state var: `atomic.views.table` (plaintext id)
- Mouse edge (LMB/RMB pressed): compose `mouse_button_down + prev var + not + and` (no `mouse_button_pressed` native)
  - **Bug fixed:** `mouse_edge` must `var_write_payload` with `id_len+id` or prev never updates → every-frame rmb thrash

## Views modularization status
- Mega `views` op-table **removed** from first-boot
- Split natives: ensure, apply_lmb, apply_rmb, pick, open_key, select_row, set_active_drag, active_key/cursor, set_cursor_active, drag_end, get/set_drag_xy, paint_links/titles/rows, …
- `render_draw` = paint_links → paint_titles → paint_rows
- `cursor_add/dec` = recipes (i32 math + block_resolve + block_instr_count + set_cursor_active)
- Hash-carrier open (RMB open target): by **instruction name** (`cond_payload`, `jump_payload`, `exec_payload`, `cond_reexec`) not stale DLL hashes
- Token color in paint_rows restored: cyan=DLL (`cvm_has_dll`), amber=block-cache hit, magenta=both; after `cvm_cache_hit` must re-resolve view key (cache_hit mutates primary_idx)
- Do **not** uget/has_override on render hot path (corrupts main conn stream)

## Registry / names
- first-boot uses `name_prefix_find` + `name_lookup` reading local **`instruction_names.bin` only**
- **User desired completion model (NOT implemented yet):**
  1. Start from `#` / `#TAG` network root
  2. `children` of node
  3. If child is a **tag** (name/content starts with `#`) → recurse
  4. Else → completion candidate token
  5. Walk whole tag graph (cache aggressively; do not block main conn per keystroke)
- Completion HUD should eventually show **path** of matched token in the tag graph

## Network model (server)
- Content-addressed blobs (hash → bytes): program/action/module blocks, DLLs, small strings
- Edge/children graph + votes
- Per-identity overrides USET/UGET: logical key → content hash (program, actions, modules, surface defs)
- Local `cache/` mirrors content; `*_ch.bin` for children when used
- Server source mirror path: `C:\Users\12159\Desktop\server`

## Performance note (drag hitch)
- Switching pan ↔ rmb used to ~1s hitch: 32-slot block LRU thrash + cold resolve did blocking uget/firstchild on main conn
- Fix in `vmstore.c`: CACHE_SLOTS **256**; resolve prefers override-cache (incl. negatives) + `cvm_children` disk cache before network

## Critical crash do-not-regress
- `cond_payload` must import **`cvm_exec` only** (not cvm_exec_payload)
- Symptom if regressed: white frame then 0xC0000005

## Important files
- `generate_atomic_first_boot.py`, `install_generic_first_boot.py`
- `atomic_module_blocks/`, `atomic_action_blocks/`, `atomic_surface_blocks/`
- `atomic_first_boot_manifest.json`, `atomic_mod_tokens.txt`, `instruction_names.bin`
- `mods_src/views_*.c`, `mods_src/views_common.h`, `mods_src/name_*.c`
- `vmstore.c` (cache/resolve), `vm.c` (`cvm_has_dll`, firstchild), `vmvar.c` (arbitrary id ≤256)
- `dxgfx.cpp` / icons
- Server mirror: `C:\Users\12159\Desktop\server`

## Verification ops
- Native change: rebuild DLL → hash-rename `mods/<sha>.dll` → update `atomic_mod_tokens.txt` / `mod_tokens.txt` → generate → install
- User must restart `vm.exe` to pick up dll/program override

## Exact next tasks (user, in order of statement)
User asked to compress context first, then implement:

1. **Pointer remap (editor multi-view):**
   - **LMB drag title** = move/drag view (was RMB drag title)
   - **RMB** = **close node** (not title-drag)
   - **RMB drag-out from row** behavior was “open linked view”; reconcile with “RMB closes node” — likely: RMB on **title/chrome closes**; row open may need another binding or still RMB-on-row opens — **confirm in implementation against user wording**: “改为右键拖出，但是不是右键拖动标题了，而是左键拖动标题，右键是关闭节点” → read as: drag-out remains a thing but title-drag is LMB; RMB closes node. Prefer: LMB title drag; LMB row select; RMB title/node close; clarify row open (maybe still RMB on row or separate). Implement LMB title-drag + RMB close first; keep row-open via explicit remaining path if needed.

2. **Input completion path display:**
   - Bottom-left completion should show the matched token’s **path** in the tag network (not only leaf name)

3. **Network token explorer (new):**
   - Same display/interaction model as the block editor views
   - Default list root: **`#TAG`** children
   - Entries can be **RMB-dragged out** into the editor space (open as views)
   - Tags (`#…`) expand via children walk; non-tags are tokens

4. **Completion data source migration (related, user-confirmed model):**
   - Replace local-only `instruction_names.bin` prefix find with tag-graph walk from `#`/`#TAG` + children (cached)

## Agent tooling
- Do not edit `ae.py`
- Compaction default: `compact_active_file` (`--active`)
