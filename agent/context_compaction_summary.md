# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
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

## Current first-boot composition (after modularization pass)
- Generator: `generate_atomic_first_boot.py`; installer: `install_generic_first_boot.py`.
- Thin program orchestrator (bare only):  
  `frame_begin → ensure → camera → frame_clear → input → zoom → camera → render_draw → hud → editor → frame_end → reexec`
- Modules (5): `camera`, `input`, `zoom`, `hud`, `editor`.
- Parts (~44): hud (`status`,`typein`,`match`), editor (`nav`,`editkeys`), input gates, pan/zoom/drag math, views surface facets (`ensure`,`pointer_lmb/rmb`,`cursor_add/dec`,`render_draw`, …).
- Actions (~13 recipes): `delete`,`insert`,`backspace`,`clear`,`click`,`rmb`,`pan`,`drag`,`begin_*`,`end_*`,`zoom_apply` — no `save`/`savekey`, no `down`/`up` alias actions, no `apply_var_edit`.
- Multi-view state var: `atomic.views.table`; camera vars `atomic.cam.x/y/z`; grab anchors for pan/drag.
- Zoom: `z' = clamp(z * (1 + notches*0.10), 0.15..6)`, mouse-centered; wheel via `g_wheel_latched` in `dxgfx` (snapshot once per frame).
- POLICY comment in generator: compose > specialized native/alias.

## Variable / display / color / icons (done)
- `vmvar`: variable **id is arbitrary-size binary** (`id` + `id_len`), not fixed 32.
- `var_set_payload`: `id_len + id + size` or `id_len + id + data` (+ legacy 32-byte fallback).
- `var_read_payload`: entire payload = id; `var_write_payload`: `id_len + id [+ data]`.
- `views_render` specialized var rows: icon + editable-looking id (+ size for set); colors:
  - cyan-ish = native DLL (`cvm_has_dll` / find cache)
  - amber-ish = resolved content in local block cache (`cvm_cache_hit`) — **not** live network uget
  - magenta = both; gray = neither
- **Do not call `cvm_has_override`/uget on render hot path** — concurrent use of main `conn` corrupts instruction stream (caused white-screen crash).
- Icons: `dxgfx_draw_icon` / `dxgfx_has_icon`; `icons/<name>.svg` or known glyphs only. **No hollow rect placeholder** for unknown tokens (user complaint fixed).
- Hit widths in `views.c` should stay conceptually ≥ render layout (swatch/name/optional icon/summary).

## Critical crash fix (must not regress)
- Stale `cond_payload.dll` imported **`cvm_exec_payload`** while source said `cvm_exec` → rewrote live cond target payloads → corrupt `ptr` → ACCESS_VIOLATION after white frame.
- Fix: rebuild `cond_payload` (and related exec path mods) so import is **`cvm_exec` only**; clear `cache/` if bad blocks suspected; reinstall first-boot.
- Symptom if regressed: window white then auto-close (`0xC0000005`).

## What was deleted/replaced as “too specialized”
- Removed: Ctrl+S `savekey`/`save`; `down`/`up` actions (nav uses `cursor_add`/`cursor_dec` facets); `var_edit_apply` + Enter apply action; `drawtext_var_xy_screen` (→ `var_read` + `drawtext_xy_stack_screen`); `measure_text_var` (→ low-level `measure_text` + `var_read`).
- Program no longer embeds raw `views`/`views_render` ops; uses bare `ensure` / `render_draw` facets.
- click/rmb_open/end_drag/begin_drag_anchor/drag_xy/sel/cur use views facets instead of raw `views_op` soup where possible.
- Disk-only leftovers (not in first-boot natives): `drawtext_var*`, `measure_text_var`, `var_edit_apply`, unused `camera_set` (stack form used).

## Still specialized / next modularization targets
User asked: are mods low-level enough? **~7/10 — orchestration good; exec primitives not fully.**
1. **Highest remaining specialized natives:** `views`, `views_render` (mega op-table DLLs; surface facets help editability but execution still integrated).
2. **Next best decomposition:** `string_append_var` / `string_backspace_var` / `string_clear_var` → pure stack string ops + `var_read`/`var_write` composition.
3. Medium: `registry_find` / `registry_token_name` (name-index convenience).
4. Minor: `mouse_button_pressed` vs edge state + `mouse_button_down`.
5. Logical recipes (actions/parts) are OK as editable compositions, not forbidden.

## Important files
- `generate_atomic_first_boot.py`, `install_generic_first_boot.py`
- `atomic_module_blocks/`, `atomic_action_blocks/`, `atomic_surface_blocks/`
- `atomic_first_boot_manifest.json`, `atomic_mod_tokens.txt`, `instruction_names.bin`, `name_cache.bin`, `mod_tokens.txt`
- `dxgfx.cpp` / `dxgfx.dll` (present IMMEDIATELY, wheel latch, icons, no empty icon box)
- `vm.c` (`find` cache, `cvm_has_dll`), `vmstore.c` (resolve, cache, `cvm_has_override` for non-hot use), `vmvar.c` (arbitrary id)
- `mods_src/views.c`, `views_render.c`, `views_open_at_row.c`, `cond_payload.c`, `measure_text.c`, `var_*_payload.c`
- `icons/*.svg`

## Verification / ops
- After native/token hash changes: rebuild DLL → hash-rename into `mods/<sha>.dll` → update `mod_tokens.txt` / `atomic_mod_tokens.txt` / `name_cache.bin` → `python generate_atomic_first_boot.py` → `python install_generic_first_boot.py`.
- `atomic_mod_tokens.txt` can override `name_cache` in `load_tokens`; keep hashes in sync with actual DLL content or first-boot will bind old natives.
- User must restart `vm.exe` to pick up new `dxgfx.dll` / mods / program override.
- Stability was OK after last install (short probes).

## Still missing vs richer multi-view editor
- Alt create child block + new view; OEM_3 / data payload insert; richer HUD/completion polish.
- Full interactive field editing for var id/size (display specialized; Enter-apply path removed as too specialized).
- Optional: purge unused specialized mod sources from disk to prevent accidental reuse.

## Agent tooling
- Do not edit `ae.py`.
- Compaction default: `compact_active_file` (`--active`); kills parent runner when `AE_RUNNER=1`.
- Legacy continue: `compact_active_file_keep_tools` / `--active-keep-tools`.

## Exact next task (user intent at compact time)
User said: **「现在压缩上下文，在新的上下文中解决这个问题」** after the modularization review.
**Next session problem to solve:** continue modularization toward lower-level mods — **priority: replace `string_*_var` with composable stack string primitives + var ops**, and/or further reduce specialized natives (`views` decomposition longer-term). Confirm with any new user message if they meant a different defect (e.g. UI bug) first.
