# Boot Editor Composition

This project now has enough generic instruction tokens to compose the first boot Toy Home and the preserved graphical self-editor as token blocks, without adding toy-shaped or editor-shaped instruction tokens.

## Token Layers

- L0 data tokens: `zero`, `one`, `two`, `four`, `eight`, `thirty_two`, `thirty_six`, `push`, `pop`, `dup`, `swap`, `over`, `rot`, `nip`, `tuck`.
- L0 numeric tokens: `add`, `sub`, `mul`, `div`, `lt`, `gt`, `le`, `ge`, `eq`, `ne`, `not`, `and`, `or`, `u64_inc`, `u64_dec`, `u64_min`, `u64_max`, `u64_mod`, `u64_bit_and`, `u64_bit_or`, `u64_bit_shl`, `u64_bit_shr`.
- L0 byte/hash tokens: `bytes_empty`, `bytes_insert`, `bytes_delete`, `bytes_replace`, `bytes_take`, `bytes_drop`, `byte_at`, `byte_put`, `hash_from_bytes`, `hash_to_bytes`, `bytes_to_hash32`, `utf8_from_codepoint`, `utf8_drop_last`.
- L0 immediate tokens: `payload_byte`, `payload_u64_le`, `payload_hash32`, `payload_bytes`, `hash_payload`.
- L1 structure tokens: `range_make`, `range_start`, `range_len`, `pair_make`, `pair_first`, `pair_second`, `rect_make`, `rect_contains`, `color_rgb`.
- L1 record tokens: `record_pack`, `record_pack_hash`, `record_pack_u64`, `record_at`, `record_token_at`, `record_payload_at`, `records_insert`, `records_replace`, `records_delete`, `records_append`, `records_count`.
- L2 state tokens: `state_hash_get`, `state_hash_set`, `state_index_get`, `state_index_set`, `state_index_inc`, `state_index_dec`, `state_record`, `state_record_replace`, `view_push`, `view_pop`.
- L2 scoped variable tokens: `var_read`, `var_write`, `var_set`, `scope_start`, `scope_end`.
- L3 platform edge tokens: `surface_open`, `surface_is_open`, `surface_clear`, `surface_rect`, `surface_frame`, `surface_line`, `surface_round_rect`, `surface_round_frame`, `surface_text`, `surface_text_utf8`, `surface_clip_push`, `surface_clip_pop`, `surface_translate_push`, `surface_translate_pop`, `surface_camera_push`, `surface_camera_pop`, `surface_poll`, `surface_char`, `surface_wheel_dir`, `surface_pos`, `surface_size`, `surface_event_clear`, `sleep_ms`, `time_ms`, `random_u64`, `load_boot`, `save_boot`, `publish_view`.
- L3 network graph tokens: `graph_children`, `graph_child_at`, `open_child`, `child_at`.

## Boot Editor Blocks

- Boot entry: the host calls the boot token once; the token block owns initialization, frame loop, event dispatch, and shutdown.
- Init phase: load the current boot hash into `boot.editor.view`, initialize the browser to the zero/public root hash, open the surface once, and then call the frame loop block.
- Frame loop: call the draw block, poll events, dispatch mouse actions, pace with `sleep_ms`, and use generic chain control to continue while the surface remains open.
- Shutdown: when `surface_poll` reports close, stop looping and close the surface instead of returning to a host-level reload loop.
- Open surface: put width and height as record payloads, decode them with `payload_u64_le`, then call `surface_open`.
- Draw frame: `surface_size`, `surface_clear`, repeated row records, `rect_make`, `color_rgb`, `surface_rect`, `surface_round_rect`, `surface_round_frame`, `surface_text`.
- Read selection: `surface_poll`, compare raw event constants, `surface_pos`, `pair_first`, `pair_second`, row `rect_contains`.
- Keep event loop stable: use `sleep_ms` after a poll/render pass so an idle editor does not busy-spin.
- Navigate view: selected row index goes through `state_index_set`, `open_child`, `view_push`, `view_pop`, or `state_hash_set`; `Root` jumps to the public root and `Catalog` jumps to the generated catalog root.
- Separate edit and browse state: keep the edited boot block in `boot.editor.view`, the edit row in `boot.editor.index`, the browser view in `boot.browser.view`, and the selected hash in `boot.browser.token` using `var_read` and `var_write`.
- Browse graph: render the selected browser hash and its first eight network children with `graph_child_at`; clicking a child restores `boot.browser.view`, sets `state_index_set`, calls `open_child`, then saves the new browser hash as both current browser view and selected hash.
- Token catalog: the builder generates a `catalog_root` block with starter fragments plus token categories (surface, record, graph, payload, state/control). A public-root edge makes the catalog discoverable, but startup still relies on the separately uploaded and voted `root -> boot_run` edge.
- Fragment blocks: generated starter fragments under `catalog_root` use only generic tokens, including rounded rectangle, text badge, and simple color/shape examples. They can be browsed, played, appended to the boot edit, and published.
- Play selected: `records_valid` distinguishes a selected block from a raw token. Blocks run through `call_stack`; raw tokens are first wrapped into a single-record block with `records_empty`, `record_pack_empty`, and `records_insert`, then run through `call_stack`.
- Add selected: `records_count` finds the append index for `boot.editor.view`. Valid blocks append as `call` records with `record_pack_hash`; raw tokens append as empty-payload records with `record_pack_empty`.
- Precise edit actions: explicit insert/replace/delete still use `boot.editor.index` for row-level control and immediately update `boot.editor.view`, `state_hash_set`, `publish_view`, and `save_boot`.
- Publish selected: valid blocks are installed as the state hash and published with `publish_view`. Raw tokens are wrapped into a single-record block, stored back to `boot.browser.token`, installed as state hash, and published without calling `save_boot`.
- Publish edit: the edited boot block is installed with `state_hash_set`, published with `publish_view`, and persisted with `save_boot`.

## Toy Home Blocks

- Default mode: the boot entry initializes `toy.mode = canvas`, opens the same surface loop, and draws the transition-style canvas first instead of the record editor.
- Transition canvas: the first surface view renders the current boot block as a node graph, draws sequence links with `surface_line`, directly expands static call/control-flow targets at an offset, shows `again_cond` as a loopback, and treats `ret` as a terminal node.
- Network graph: the public/network view is a canvas node. Clicking it calls `graph_children` once and caches the result; rendered child nodes use `child_at`, and clicking a child opens that child and refreshes the cached children.
- Inspector mode: the previous editor draw and mouse dispatch are still present behind `toy.mode = inspector`, with a `返回玩具` action back to Toy Home.
- Stage data: `toy.stage.data` is a record chain of `noop` records. Each payload is a 128-byte object descriptor with type, local x/y/w/h, color, radius or variant, flags or seed, primary hash, and auxiliary hash fields.
- Stage rendering: Toy Home draws a clipped stage, translates into stage-local coordinates, stores `toy.render.data`, loops over `records_count`, parses each object payload with byte/u64 primitives, and dispatches object renderers by type.
- Object types: solid rectangles, rounded rectangles, frames, UTF-8 text stickers, badges, clipped/translated external block wrappers, and animated pulse objects are generated as normal token blocks.
- Stage editing: add, select, drag, recolor, resize, duplicate, undo, clear, and text-edit actions are composed from `records_at`, `record_payload_at`, `u64_le_read`, `u64_le_put`, `bytes_replace`, `record_pack`, and `records_replace`.
- Chinese text: labels and editable sticker/title text use `surface_text_utf8`, `surface_char`, `utf8_from_codepoint`, and `utf8_drop_last` so UTF-8 text is preserved instead of using ANSI drawing.
- Explore panel: Toy Home keeps `toy.browser.view` and `toy.browser.selected` separate from Inspector browser state, and links a generated `toy_gallery_root` under the public root without voting it above `boot_run`.
- External wrappers: selected external blocks can be added as stage objects. They are executed inline through `call_stack` inside `surface_clip_push` and `surface_translate_push` contexts, so drawing tokens are constrained to the object surface when they respect the generic surface context.
- Publishing: `发布` builds a `CVM_TOY_STAGE_V1` runner block, uploads it, publishes it, links/votes it under `toy_gallery_root`, and appends it to the user shelf key without calling `save_boot`.
- Boot actions: `设为启动` builds a runner and calls `save_boot`; `发布并设为启动` publishes first, then persists the generated runner as the user's boot.
- Runtime renderer: published toy packages call `toy_runtime_render_stage`. If a surface already exists, it draws once for inline previews. If no surface exists, it opens a standalone frame loop and redraws until close.

## Confirmation

No toy-specific or boot-editor-specific instruction token is required. Toy Home, public-root browsing, catalog jumps, starter fragments, play, append, raw-token wrapping, stage editing, text editing, external object clipping, and publish actions are all block-level composition over generic tokens: numbers, stack shuffling, UTF-8 bytes, records, rectangles, surface events, surface context, state, variables, call control, and graph publishing.

The boot editor must remain a token-block program. If a future first-start flow cannot be composed comfortably from the current token set, fill the smallest reusable L0-L3 gap first rather than adding an application-shaped instruction or relying on the host to repeatedly restart the boot block.
