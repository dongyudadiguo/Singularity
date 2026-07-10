# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
- Preserve registered 32-byte `id.bin` identity beginning `5673fae3`; never replace with rejected identity beginning `66ee6f28`.
- Worktree is dirty. `agent/input.json` is runner-managed; do not revert unrelated changes.
- Do not modify `agent/ae.py` unless the user explicitly asks.
- User tests `vm.exe` manually; do not auto-launch it from agent (console popups interfere).
- Answer the user in Chinese when they ask `用中文回答` or prefer Chinese.

## Architecture truths
- Fixed bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`.
- VM block format: `token[32] + payload_size[u32 LE] + payload`, ending with zero token.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild. Naming does not distinguish DLL vs block.
- Modularization (user definition): first program is a thin orchestrator of multiple non-DLL logical token blocks placed bare in the stream (empty payload). Prefer bare tokens over `const+exec`. Prefer stream-embedded blocks over `exec`. Use `cond_payload` for key-triggered actions.
- Do not use `cvm_exec_payload` from frame/program conditionals: it rewrites live payload key->content hash and can corrupt the reexecing program cache / async-write a bad override. `cond_payload` must call non-mutating `cvm_exec(token)`.
- `var_set_payload` trap: payload `id[32] + u32` with total size exactly 36 means ALLOCATE that many zero bytes, not write 4 data bytes.
- First boot must stay small general-purpose atomic mods. Never restore integrated `ui_*`, `uistate`, `editor_frame`, or `editor_init`.
- Decomposability rule (user): tokens should be as editable/split as possible; specialized/integrated tokens especially need firstchild token blocks / surfaces.

## Current first-boot composition
- Generator: `generate_atomic_first_boot.py`; installer: `install_generic_first_boot.py`.
- Thin program orchestrator: bare module tokens, no exec.
- Frame order: `frame_begin` → views ensure → `camera` → `frame_clear` → `input` → `zoom` → `camera` → `views_render` → `hud` → `editor` → `frame_end` → `reexec`.
- Modules: `camera`, `input`, `zoom`, `hud`, `editor`.
- Decomposed parts under `atomic_module_blocks/`:
  - `hud` → `status` + `typein` + `match`
  - `editor` → `nav` + `editkeys` + `savekey`
  - `input` → `click_on` + `rmb_on` + `pan_on` + `drag_on`
  - `pan` → ensure + `pan_x` + `pan_y`
  - `drag` → ensure + `drag_xy`
  - `rmb` → `rmb_open` + `begin_drag_anchor`
  - `zoom_apply` → `zoom_z` + `zoom_x` + `zoom_y`
- Actions still include editor core + pan/drag/click/rmb/zoom_apply.
- Multi-view state var: `atomic.views.table`; camera vars `atomic.cam.x/y/z`; absolute grab anchors for pan/drag.
- Zoom formula: `z' = clamp(z * (1 + notches*0.10), 0.15..6)`, mouse-centered via world mouse; only when wheel ≠ 0.
- `instruction_names.bin` carries natural names for natives + modules + parts + facets.

## Specialized native surfaces
- Problem: RMB-open on native `views` resolved name-string firstchild / PE bytes → empty content.
- Fix: native surfaces in `atomic_surface_blocks/`; installer sets override native token → surface definition block.
- `views` surface: 27 bare facet tokens (`ensure`, `pointer_lmb`, `pointer_rmb`, drag/cursor ops, etc.).
- `views_render` surface: `render_draw`.
- Open rule in `views` / `views_open_at_row`: only hash-carriers (`cond_payload`, `jump_payload`, `exec`, `exec_payload`) open a 32-byte payload hash; ordinary natives open themselves so surface override is visible.
- Exec still uses `find(native DLL)`; editor resolve uses override.

## Performance / latency fixes (done)
- Removed per-HIT/MISS `printf` from `cvm_resolve_payload_hash` (main FPS killer under modular resolve).
- Cached `find()` LoadLibrary/GetProcAddress results in `vm.c` (256-slot map, negatives cached).
- Hashed name index in `views` / `views_render`.
- Present: `D2D1_PRESENT_OPTIONS_IMMEDIATELY` (cut vsync input-to-photon lag).
- Resize render target only on client size change.
- Cache DWrite text formats by quantized size.
- User confirmed FPS and latency OK after these.

## Wheel zoom history and critical bug fix
- Original empty-scroll cause: `GET_WHEEL_DELTA_WPARAM(w) / 120` integer truncate dropped high-res partial notches.
- Changed to accumulate raw WHEEL_DELTA; `mouse_wheel` pushes f32 notches via `dxgfx_mouse_wheel_f`.
- Second empty-scroll cause: `frame_end` cleared wheel after zoom had run, discarding mid-frame deltas.
- Introduced `g_wheel_accum` + `g_wheel_frame` snapshot.
- **Broken “can’t scroll at all” bug:** `frame_clear` re-enters `frame_begin` while already drawing; second snapshot saw empty accum and zeroed `g_wheel_frame` before zoom.
- **Fix applied and rebuilt:** `g_wheel_latched` — snapshot only once per frame; clear latch in `frame_end`. `dxgfx.dll` rebuilt successfully.
- User must restart `vm.exe` to pick up `dxgfx.dll` latch fix if not already verified after restart.

## Important files
- `generate_atomic_first_boot.py`, `install_generic_first_boot.py`
- `atomic_module_blocks/`, `atomic_action_blocks/`, `atomic_surface_blocks/`
- `atomic_first_boot_manifest.json`, `atomic_mod_tokens.txt`, `instruction_names.bin`
- `dxgfx.cpp` / `dxgfx.dll` (present, wheel latch, text format cache)
- `vm.c` (find cache), `vmstore.c` (no hot-path printf, 32 cache slots, pin)
- `mods_src/views.c`, `views_render.c`, `views_open_at_row.c`, `mouse_wheel.c`

## Still missing vs reference multi-view editor
- Alt create child block + new view
- OEM_3 / data payload insert
- Richer status HUD / completion polish
- Verify after full `vm.exe` restart: wheel zoom works and is smooth (no empty scrolls, no dead wheel after latch fix); names; input prefix match; MMB pan stable; mouse move no list drift
- Do not reintroduce integrated UI DLLs

## Agent viewer / tooling notes
- Do not edit `ae.py`.
- Compaction default: compact-and-stop via `compact_active_file` (`--active`); kills parent runner when `AE_RUNNER=1`.
- Legacy continue path: `compact_active_file_keep_tools` / `--active-keep-tools`.
