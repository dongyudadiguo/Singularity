# Compacted Engineering Context

## Workspace and constraints
- Project: `C:\Users\12159\Desktop\Singularity`; live server `118.25.42.70:9000`.
- Preserve registered 32-byte `id.bin` identity beginning `5673fae3`; never replace with rejected identity beginning `66ee6f28`.
- Worktree is dirty. `agent/input.json` is runner-managed; do not revert unrelated changes.
- Do not modify `agent/ae.py` unless the user explicitly asks.
- VM block format: `token[32] + payload_size[u32 LE] + payload`, ending with zero token.
- Native mod DLL names are SHA-256 content hashes. Logical keys resolve by user override then graph child.
- First boot must stay small general-purpose atomic mods. Never restore integrated `ui_*`, `uistate`, `editor_frame`, or `editor_init`.
- User tests `vm.exe` manually; do not auto-launch it from agent (console popups interfere).

## Architecture truths
- Fixed bootstrap token: `46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed`.
- Program logical key: `2c4ffa37caa880f5820f2ece9a03ea13ead353229813bd6930d395945bff7f6d`.
- `generate_atomic_first_boot.py` builds first/program/action/module blocks + `atomic_first_boot_manifest.json` + local `instruction_names.bin`.
- `install_generic_first_boot.py` validates hash-named natives, uploads DLLs/blocks, sets overrides, votes bootstrap child; supports modular `modules` map.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild. Naming does **not** distinguish DLL vs block (`camera`/`pan` not `mod.camera`/`action.pan`).
- **Modularization (user definition)**: first program is a thin orchestrator of multiple **non-DLL logical token blocks** placed **bare in the stream** (empty payload). Prefer bare tokens over `const+exec`. Prefer stream-embedded blocks over `exec`. Use `cond_payload` for key-triggered actions.
- **Do not use `cvm_exec_payload` from frame/program conditionals**: it rewrites live payload key->content hash and can corrupt the reexecing program cache / async-write a bad override. `cond_payload` must call non-mutating `cvm_exec(token)`.
- `var_set_payload` trap: payload `id[32] + u32` with total size exactly 36 means ALLOCATE that many zero bytes, not write 4 data bytes.

## Current deployed first boot (latest install)
- first_hash: `bbed82e93278b11605027ec5f3fcabede4a19f458ad722ec9392f2bf77e3309e`
- program_hash: `57a91b0634f8a1a6ef8524956c81acb892995b64dfbbe8e3dbbd7c0ad4c87a27` (12 orchestrator instructions, bare module tokens, no exec)
- composition: bare logical module tokens in stream; `modules_inlined: false`
- Modules (logical key -> content): `camera`, `input`, `zoom`, `hud`, `editor` under `atomic_module_blocks/`
- Actions: editor core + `begin_pan/end_pan/begin_drag_anchor/pan/click/rmb/drag/end_drag/zoom_apply`
- Multi-view state var: `atomic.views.table`
- Camera vars: `atomic.cam.x/y/z`
- Absolute-grab anchors: `atomic.cam.grab_mx/my`, `grab_cx/cy`, `grab_vx/vy`, flags `pan_active`, `drag_anchored`
- Frame orchestrator shape:
  - frame_begin → views ensure → camera → frame_clear → input → zoom → camera → views_render → hud → editor → frame_end → reexec
- Zoom: module only runs `mouse_wheel` + `cond_payload(zoom_apply)` when wheel ≠ 0 (fixed mouse-wobble cam drift from always rewriting cam via world_mouse)
- Zoom apply: factor `1+wheel*0.08`, clamp 0.15..6, mouse-centered `cam' = world + (cam-world)*(old/new)`
- Pan/drag: absolute grab anchors (MMB pan, RMB drag via views op32/33)
- Hit/display: text-width based (measure token name + trailing payload summary); not fixed 520px
- HUD screen-space; match label trails input via `measure_text_var` + `registry_find` prefix match
- `instruction_names.bin` ~130 natural names (natives + modules + actions); required for list names and input match

## Important natives / infra of note
- `cond_payload` (safe non-mutating): pinned after rewrite to `cvm_exec`
- `views` multi-op; ops include 29 LMB, 30 RMB open, 32 get_drag_xy, 33 set_drag_xy absolute; text-width hit uses `dxgfx_measure_text` + name index
- `views_render`: text-width layout; loads `instruction_names.bin`
- RMB open payload-hash rule: if instruction `payload_size == 32`, open that payload hash; else open row token
- Helpers: `mouse_f`, `measure_text_var`, `screen_size`, `block_select_stack`, `draw*_screen`
- **vmstore**: `CACHE_SLOTS` 32; live-frame **pin** via `cvm_cache_pin_base/unpin` from `vmstate` on set_current/ret/replace; recv failures no longer `exit(1)`
- `build_vmstate.bat` links `libvmstore.a` for pin imports
- Recent rebuilt natives (hash names in `atomic_mod_tokens.txt`): `views`, `views_render`, `registry_find`, `registry_token_name`, `drawtext_xy_stack_screen` (draw C-string length not full 96)

## Bugs fixed this session
- Token hit/display fixed-width → text-width; payload trails name
- Wheel zoom coarse/always-on mouse-center → wheel-gated `zoom_apply` only
- MMB pan crash/exit: cache thrash + nested frames + recv exit; pin + 32 slots + safer recv; modular bare tokens still need pin
- Mouse wobble shifted token list: zoom rewrote cam every frame; now no-op when wheel==0
- Token names / input match: enrich names table, fix match-label draw length, natural names (no mod./action. prefix)
- Modularization: bare logical tokens in program stream (not one giant block, not const+exec)

## Still missing / next vs reference multi-view editor
- Alt create child block + new view
- OEM_3/data payload insert
- Richer status HUD / completion polish
- Verify after full `vm.exe` restart: names show, input prefix match, MMB pan stable, mouse move no list drift, wheel zoom mouse-centered
- Do not reintroduce integrated UI DLLs

## Agent viewer / tooling notes
- Do not edit `ae.py`.
- Compaction default: compact-and-stop via `compact_active_file` (`--active`); kills parent runner when `AE_RUNNER=1`.
- Legacy continue path: `compact_active_file_keep_tools` / `--active-keep-tools`.
