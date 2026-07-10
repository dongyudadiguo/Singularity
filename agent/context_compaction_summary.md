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
- `generate_atomic_first_boot.py` builds first/program/action blocks + `atomic_first_boot_manifest.json` + local `instruction_names.bin`.
- `install_generic_first_boot.py` validates hash-named natives, uploads DLLs/blocks, sets overrides, votes bootstrap child.
- Token dual identity: same token may be native DLL if platform supports it, else logical block via resolve/override/firstchild.
- Prefer stream-embedded blocks over `exec` for modularization. Use `cond_payload` for key-triggered actions.
- **Do not use `cvm_exec_payload` from frame/program conditionals**: it rewrites live payload key->content hash and can corrupt the reexecing program cache / async-write a bad override. `cond_payload` must call non-mutating `cvm_exec(token)`.
- `var_set_payload` trap: payload `id[32] + u32` with total size exactly 36 means ALLOCATE that many zero bytes, not write 4 data bytes.

## Current deployed first boot (latest install)
- first_hash: `c245a2d38044a0599a91ecd3937441a91fe226c62e2dfbd852314196f4767301`
- program_hash: `8b295b07345fe92dbdfc6f8c24fc99d008ac050a55a253d77231ed1981378a20` (~82 instructions)
- Actions: `down/up/delete/insert/backspace/clear/save` plus `begin_pan/end_pan/begin_drag_anchor/pan/click/rmb/drag/end_drag`
- Multi-view state var: `atomic.views.table`
- Camera vars: `atomic.cam.x/y/z`
- Absolute-grab anchors: `atomic.cam.grab_mx/my`, `grab_cx/cy`, `grab_vx/vy`, flags `pan_active`, `drag_anchored`
- Frame behavior:
  - ensure/seed view0 if empty
  - camera from vars; wheel zoom clamp 0.15..6
  - **absolute grab pan (MMB)**: first held frame captures mouse0+cam0; while held `cam = grab_cam - (mouse - grab_mouse) / zoom`; release clears `pan_active`
  - **absolute grab view drag (RMB held)**: after rmb open/title, capture mouse0+view0; while held `view = grab_view + (mouse - grab_mouse) / zoom` via views op33; release end_drag
  - LMB select via views op29; RMB open via views op30
  - `views_render` draws all views + link lines
  - keyboard editor actions on active view key/cursor
  - HUD is **screen-space** (`drawtext_screen` / `drawtext_var_xy_screen` / `drawtext_xy_stack_screen`): top-left fixed, bottom uses `screen_h - 52`
  - match label trails input via `measure_text_var`
- Installer min instruction check: 40
- Local `dxgfx.dll` rebuilt with: DPI awareness, close window -> DestroyWindow + ExitProcess in frame_begin, client-pixel mouse space, `dxgfx_mouse_f`, `dxgfx_draw_text_screen`

## Important natives / tokens of note
- `cond_payload` (safe non-mutating): pinned in `atomic_mod_tokens.txt` after rewrite to `cvm_exec`
- `views` multi-op table native; recent ops include:
  - 12 drag_step (delta)
  - 13 drag_end
  - 29 pointer_lmb
  - 30 pointer_rmb open/drag
  - **32 get_drag_xy**
  - **33 set_drag_xy_stack absolute**
- **RMB open payload-hash rule**: if instruction `payload_size == 32`, open that payload hash (for `cond_payload`/`jump_payload`/`exec_payload`); else open row token
- Helpers: `mouse_f`, `measure_text_var`, `screen_size`, `block_select_stack`, `draw*_screen` variants
- `build_mods.bat` may need entries for new screen/draw mods when full rebuild is used; recent builds often compiled targeted mods via gcc + hash rename

## Pan/drag diagnosis history (resolved direction)
- User observed non-1:1 then clarified it felt like **damping**: move freely then return mouse to press point, view does **not** restore.
- Root cause of damping feel: frame-delta sampling (`mouse - last`) with last written end-of-frame loses mid-frame motion; path integral not conserved.
- Fix direction implemented: **absolute grab anchors**, not delta accumulation.
- User must fully restart `vm.exe` after first_hash changes (new grab/active vars allocated in first block).

## Still missing / next vs reference multi-view editor
- Alt create child block + new view
- OEM_3/data payload insert
- Richer status HUD / completion polish
- Verify absolute pan/drag after manual VM restart (mouse return-to-press should restore view)
- Do not reintroduce integrated UI DLLs

## Agent viewer / tooling notes
- Do not edit `ae.py`.
- Compaction default: compact-and-stop via `compact_active_file` (`--active`); kills parent runner when `AE_RUNNER=1`.
- Legacy continue path: `compact_active_file_keep_tools` / `--active-keep-tools`.
