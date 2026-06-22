# Boot Editor Composition

This project now has enough generic instruction tokens to compose the first boot graphical self-editor as token blocks, without adding editor-shaped instruction tokens.

## Token Layers

- L0 data tokens: `zero`, `one`, `two`, `four`, `eight`, `thirty_two`, `thirty_six`, `push`, `pop`, `dup`, `swap`, `over`, `rot`, `nip`, `tuck`.
- L0 numeric tokens: `add`, `sub`, `mul`, `div`, `lt`, `gt`, `le`, `ge`, `eq`, `ne`, `not`, `and`, `or`, `u64_inc`, `u64_dec`, `u64_min`, `u64_max`, `u64_bit_and`, `u64_bit_or`, `u64_bit_shl`, `u64_bit_shr`.
- L0 byte/hash tokens: `bytes_empty`, `bytes_insert`, `bytes_delete`, `bytes_replace`, `bytes_take`, `bytes_drop`, `byte_at`, `byte_put`, `hash_from_bytes`, `hash_to_bytes`, `bytes_to_hash32`.
- L0 immediate tokens: `payload_byte`, `payload_u64_le`, `payload_hash32`, `payload_bytes`, `hash_payload`.
- L1 structure tokens: `range_make`, `range_start`, `range_len`, `pair_make`, `pair_first`, `pair_second`, `rect_make`, `rect_contains`, `color_rgb`.
- L1 record tokens: `record_pack`, `record_pack_hash`, `record_pack_u64`, `record_at`, `record_token_at`, `record_payload_at`, `records_insert`, `records_replace`, `records_delete`, `records_append`, `records_count`.
- L2 state tokens: `state_hash_get`, `state_hash_set`, `state_index_get`, `state_index_set`, `state_index_inc`, `state_index_dec`, `state_record`, `state_record_replace`, `view_push`, `view_pop`.
- L2 scoped variable tokens: `var_read`, `var_write`, `var_set`, `scope_start`, `scope_end`.
- L3 platform edge tokens: `surface_open`, `surface_clear`, `surface_rect`, `surface_frame`, `surface_round_rect`, `surface_round_frame`, `surface_text`, `surface_poll`, `surface_pos`, `surface_size`, `surface_event_clear`, `sleep_ms`, `load_boot`, `save_boot`, `publish_view`.
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

## Confirmation

No boot-editor-specific instruction token is required. Public-root browsing, catalog jumps, starter fragments, play, append, raw-token wrapping, and publish actions are all block-level composition over generic tokens: numbers, stack shuffling, bytes, records, rectangles, surface events, state, variables, call control, and graph publishing.

The boot editor must remain a token-block program. If a future first-start flow cannot be composed comfortably from the current token set, fill the smallest reusable L0-L3 gap first rather than adding an application-shaped instruction or relying on the host to repeatedly restart the boot block.
