# First Boot Toy Home Plan

## Goal

Turn the first-start program from a hash/record tool into a real toy without changing the VM startup architecture, network protocol, block format, or publishing model. The result should open into a Chinese-first visual sandbox where users can explore, play, experiment, remix, and publish quickly. The existing hash/record editor remains fully available as an Inspector panel.

## Locked Decisions

- Keep the current boot architecture: root still resolves to `boot_run`, and `boot_run` still loads the current user's boot hash.
- Keep the first-start program as generated token blocks in `boot_editor_builder.go`; do not move UI behavior into host C or VM code.
- Preserve all current editor abilities: graph browsing, token catalog, row selection, insert call, insert token, replace call, replace token, delete, publish edit, and save boot.
- Default screen becomes Toy Home, not the record editor.
- UI is Chinese-first. Inspector can keep English technical labels where useful.
- Add generic native instruction tokens where needed. Do not add toy/editor/boot-specific native tokens.
- `玩一下` uses current inline execution with `call_stack`. Do not implement process sandbox in this pass.
- `发布` does not overwrite boot. Provide separate `设为启动` and `发布并设为启动` actions.
- External published blocks added to the stage are wrapped as objects and replayed inside a translated/clipped surface context.
- Include deployment after validation: dry-run, build tokens, run local checks, then upload and set the current `id.bin` user's boot.

## Source Boundaries

Edit source files during implementation, not during planning:

- `boot_editor_builder.go`: replace the default generated UI with Toy Home, add stage/object/publish blocks, and move the old editor into Inspector mode.
- `mods_src/surface_ops.h`: add shared generic surface context and Unicode event handling.
- Existing `mods_src/surface_*.c`: make drawing, clear, size, and position context-aware.
- New generic token sources in `mods_src`: add the primitives listed below.
- `mods_map.txt` and `mods/*.dll`: update by rebuilding native tokens.
- `BOOT_EDITOR_COMPOSITION.md` and `INSTRUCTION_TOKEN_GUIDELINES.md`: update only after implementation to document the new generic primitives and Toy Home composition.

Do not change:

- `vm.c` startup walk logic.
- `server.go` network protocol or data model.
- Existing block binary format.
- Existing root-to-`boot_run` startup invariant.

## Generic Native Tokens To Add

Add these as reusable primitives, not app-specific instructions:

- `surface_text_utf8`: same stack contract as `surface_text`; draw a bytes block as UTF-8 text by converting to UTF-16 and using wide GDI text output.
- `surface_clip_push`: pop a rect in current local coordinates, intersect it with the current clip, and push the resulting clip context.
- `surface_clip_pop`: restore the previous clip context.
- `surface_translate_push`: pop `x`, `y` offsets and push a translated coordinate context.
- `surface_translate_pop`: restore the previous translation context.
- `surface_char`: return the current Unicode character codepoint when the last surface event is text input, otherwise zero.
- `time_ms`: push a monotonic millisecond timestamp.
- `random_u64`: push a random 64-bit value using a platform RNG.
- `u64_mod`: pop divisor and dividend, push `dividend % divisor`, returning zero for divisor zero.
- `utf8_from_codepoint`: pop a Unicode codepoint and push a bytes block containing its UTF-8 encoding.
- `utf8_drop_last`: pop a UTF-8 bytes block and push a block with the final codepoint removed without corrupting multibyte text.

Surface context implementation notes:

- Prefer a separate shared mapping such as `Local\\CVM_Surface_Context` instead of expanding `CvmState`, so the VM state layout is not changed.
- `surface_open` should register/use wide window APIs (`RegisterClassW`, `CreateWindowW`) so `WM_CHAR` carries Unicode characters for IME input.
- `surface_poll` may continue returning the event code, but the surface context should also retain the last Unicode char for `surface_char`.
- `surface_rect`, `surface_frame`, `surface_round_rect`, `surface_round_frame`, `surface_text`, `surface_text_utf8`, and `surface_clear` must apply current translation and clipping.
- `surface_size` should return the current local clipped size when a clip is active.
- `surface_pos` should return mouse coordinates transformed into current local coordinates.

## Native Build Plan

- Rebuild all surface-related tokens and all new tokens after changing `surface_ops.h`.
- Prefer a full `build_mods.ps1` rebuild if header dependencies become hard to track.
- Keep old DLL hash files in `mods/`; existing published blocks can still reference old token hashes.
- Ensure `mods_map.txt` points to the newly compiled token hashes before running `boot_editor_builder.go`.
- Restart `vm.exe` after rebuilding native tokens to avoid stale loaded DLLs or stale shared mappings.

## Stage Data Model

Use a dual-form model so the stage is both editable and publishable:

- `toy.stage.data`: editable object data block.
- `toy.stage.prev`: previous data hash for undo.
- `toy.stage.selected_index`: selected object record index.
- `toy.stage.dragging`: drag state.
- `toy.stage.drag_dx` and `toy.stage.drag_dy`: click offset inside selected object.
- `toy.title.bytes`: UTF-8 title/name bytes for publishing.
- `toy.browser.view`: current explore graph parent.
- `toy.browser.selected`: selected external block/package.
- `toy.mode`: home, explore, inspector, catalog, text-edit, or publish panel.
- `toy.message`: status text hash for publish/save/add feedback.

Represent `toy.stage.data` as a record chain of `noop` records. Each record payload is a fixed-layout object descriptor:

- offset 0: `type` as u64.
- offset 8: `x` as u64.
- offset 16: `y` as u64.
- offset 24: `w` as u64.
- offset 32: `h` as u64.
- offset 40: `color` as u64/COLORREF.
- offset 48: `radius_or_variant` as u64.
- offset 56: `flags_or_seed` as u64.
- offset 64: `ref_hash` as 32 bytes, used for text bytes or external block hash.
- offset 96: `aux_hash` as 32 bytes, reserved for secondary text/data.

Initial object types:

- `1`: solid rectangle.
- `2`: rounded rectangle.
- `3`: frame.
- `4`: text sticker using `surface_text_utf8`.
- `5`: badge/card using shape plus text.
- `6`: external block wrapper, rendered with `surface_clip_push` and `surface_translate_push`, then `call_stack`.
- `7`: animated/randomized visual object using `time_ms` and object seed.

## Toy Home UI

Default surface remains a single token-block app, but the first draw is Toy Home:

- Header: Chinese title and short instruction, for example “点一个玩具，摆弄一下，觉得有意思就发布。”
- Main stage: large canvas area that renders `toy.stage.data`.
- Toybox strip: cards for shape, rounded block, frame, sticker, badge, sparkle/random object, animated object, and external selected block.
- Explore panel: shows toy gallery and public root entries with title when the item is a Toy package, otherwise short hash and explicit play controls.
- Action bar: `玩一下`, `加到舞台`, `拖拽`, `换色`, `变大`, `变小`, `复制`, `撤销`, `清空`, `发布`, `设为启动`, `发布并设为启动`.
- Advanced entry: `Inspector`, `Token Catalog`, and `Root/Public` shortcuts.

Do not show full hashes, record rows, or insert/replace/delete controls on the default path. Keep those in Inspector.

## Inspector UI

Move the current editor UI into `toy.mode = inspector` with minimal behavioral changes:

- Preserve left edit records and right graph children.
- Preserve `Append selected`, `Insert call`, `Insert token`, `Replace call`, `Replace token`, `Delete`, `Publish edit`, `Root`, `Catalog`, `Back`, `Play selected`, and `Publish selected`.
- Add `返回玩具` to go back to Toy Home.
- Keep full hashes and record counts here.
- Use `surface_text_utf8` for Chinese labels where practical, but do not rewrite all technical text if it slows implementation.

## Rendering And Interaction Blocks

Add generated token blocks in `boot_editor_builder.go`:

- `toy_init_vars`: initializes boot editor state, toy stage, title, browser, mode, dirty flag, and root/catalog/gallery hashes.
- `toy_draw_home`: clears screen and draws the stage, toybox, explore panel, and action bar.
- `toy_draw_stage`: draws stage background and calls the stage object render loop.
- `toy_render_stage_loop`: loops over `records_count(toy.stage.data)` and calls `toy_render_object` for each object.
- `toy_render_object`: parses object payload fields and dispatches by object type.
- `toy_render_external_object`: pushes clip/translate context, calls the external block inline with `call_stack`, pops contexts, and continues.
- `toy_hit_test_loop`: loops over objects on mouse down, uses each object rect and final hit wins so later objects behave as topmost.
- `toy_drag_update`: updates selected object's x/y fields while dragging.
- `toy_add_shape`, `toy_add_round`, `toy_add_frame`, `toy_add_sticker`, `toy_add_badge`, `toy_add_random`, `toy_add_animated`, `toy_add_external`: append object records to `toy.stage.data` after saving `toy.stage.prev`.
- `toy_mutate_color`, `toy_mutate_size`, `toy_duplicate_selected`, `toy_undo`, `toy_clear`: stage editing helpers.
- `toy_text_input_dispatch`: handles `WM_CHAR`, appends UTF-8 using `utf8_from_codepoint`, and handles backspace using `utf8_drop_last`.
- `toy_play_selected_inline`: current `call_stack` behavior for selected external block/package.
- `toy_dispatch_mouse`: handles Toy Home hit areas, drag start/update/end, action buttons, mode switches, and explore selection.
- `toy_frame_loop`: replaces or wraps the current frame loop and dispatches based on `toy.mode`.

## Publishing Model

Publishing builds an executable Toy package block from the editable stage data:

- Build a runner block dynamically with records that set runtime variables and call a generic renderer.
- Runner block format:
  - record 0: `noop` payload `CVM_TOY_STAGE_V1`.
  - record 1: `payload_hash32` payload `toy.stage.data`.
  - record 2: `var_write` payload `toy.runtime.stage.data`.
  - record 3: `payload_hash32` payload `toy.title.bytes`.
  - record 4: `var_write` payload `toy.runtime.title.bytes`.
  - record 5: `call` payload `toy_runtime_render_stage`.
- `toy_runtime_render_stage` reads `toy.runtime.stage.data` and renders the same object model outside the editor.
- Include metadata noops only if they do not break direct execution.

Publish action behavior:

- `发布`: builds runner, sets `state_hash` to runner, calls `publish_view`, links runner under `toy_gallery_root`, votes `toy_gallery_root -> runner`, and appends runner to the user's shelf key. It does not call `save_boot`.
- `设为启动`: sets `state_hash` to the current stage runner and calls `save_boot`; it may publish only if user clicked `发布并设为启动`.
- `发布并设为启动`: performs `发布`, then `save_boot` for the generated runner.
- Do not vote `root -> runner`; this protects the root startup order where `boot_run` must remain the top startup child.

Use a fixed per-user key for the shelf, for example a 32-byte constant derived from `CVM_TOY_SHELF`. Store the shelf as a record chain of `noop` records with runner hash payloads.

## Explore Model

- Add a generated `toy_gallery_root` block and link it from public root without voting it above `boot_run`.
- Keep `catalog_root` and token categories for Inspector/Catalog.
- Explore panel tabs: `玩具流`, `公开根`, `我的架子`, `Catalog`.
- Auto-preview known Toy packages by parsing `CVM_TOY_STAGE_V1` and rendering their data model in a clipped card.
- For arbitrary raw external blocks, show a card with title fallback/short hash and explicit `玩一下` / `加到舞台` actions.
- Provide an optional advanced toggle/action to inline-preview arbitrary external blocks in clipped cards; because inline execution was chosen, document that raw blocks can still hang or mutate state.

## Initial Toy Content

Add generated starter toys under the toybox/gallery:

- Color block: draggable solid rectangle.
- Soft block: rounded rectangle with radius controls.
- Frame block: outlined rectangle/card frame.
- Text sticker: editable UTF-8 text object.
- Badge: rounded card plus editable title text.
- Sparkle: several small color objects with random seed.
- Pulse: animated object using `time_ms`.
- External remix slot: wraps current explore selection as a stage object.

These should be normal blocks/packages where possible, browsable, playable, addable, publishable, and remixable.

## Rollout Steps

1. Implement and compile generic native tokens.
2. Rebuild `mods_map.txt` and required DLLs.
3. Update `boot_editor_builder.go` to generate Toy Home, Inspector mode, runtime renderer, gallery, starter toys, and package publishing blocks.
4. Run `go run .\boot_editor_builder.go -dry-run`.
5. Confirm dry-run output includes new blocks for Toy Home, Inspector entry, runtime renderer, toy gallery root, publishing helpers, object renderers, text input, and all starter toys.
6. Verify all token names referenced by the builder exist in `mods_map.txt`.
7. Run local `vm.exe` after restart and manually check the UI.
8. Run `go run .\boot_editor_builder.go` to upload blocks/edges and set the current `id.bin` user's boot.
9. Restart `vm.exe` and verify Toy Home is the first screen.
10. Record the previous boot hash from dry-run/deploy output or from `load_boot` before overwrite for rollback.

## Validation Checklist

- First screen is Toy Home, not a hash/record editor.
- Chinese labels render correctly.
- Chinese text input works for sticker/title, including backspace.
- Toybox cards add editable objects to the stage.
- Stage objects can be selected, dragged, recolored, resized, duplicated, undone, and cleared.
- Animated/random objects visibly change using `time_ms` or `random_u64`.
- External block objects render inside their clipped/translated stage rectangle.
- `玩一下` still runs selected blocks inline through `call_stack`.
- `发布` creates an executable package, links it under public root and toy gallery, votes only under toy gallery, and does not call `save_boot`.
- `设为启动` changes boot only when explicitly clicked.
- `发布并设为启动` publishes and saves boot.
- Inspector preserves all previous editor actions.
- Catalog remains reachable.
- Root startup still resolves to `boot_run` after deployment.

## Accepted Risks And Explicit Solutions

- Inline execution can hang, mutate state, or draw over the UI. This is accepted for this pass because inline execution was chosen. If it becomes unacceptable, implement the process-sandbox `call_isolated` option as a follow-up without changing block format or publishing protocol.
- Auto-previewing arbitrary external blocks is unsafe with inline execution. The plan auto-previews known Toy packages and provides explicit play/add for raw blocks, plus an advanced raw-preview path.
- Rebuilding native tokens changes token hashes. Keep old hash DLLs so old published blocks continue to run, and ensure generated new blocks use the updated `mods_map.txt`.
- Surface clip/translate must be applied consistently to all surface drawing tokens; missing one will allow external blocks to escape their card/stage bounds.
- Public root is also part of VM startup discovery. Never vote newly published user toys above `boot_run` on root.
