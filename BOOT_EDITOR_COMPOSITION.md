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
- L3 platform edge tokens: `surface_open`, `surface_clear`, `surface_rect`, `surface_frame`, `surface_text`, `surface_poll`, `surface_pos`, `surface_size`, `surface_event_clear`, `sleep_ms`, `load_boot`, `save_boot`, `publish_view`.
- L3 network graph tokens: `graph_children`, `open_child`, `child_at`.

## Boot Editor Blocks

- Open surface: put width and height as record payloads, decode them with `payload_u64_le`, then call `surface_open`.
- Draw frame: `surface_size`, `surface_clear`, repeated row records, `rect_make`, `color_rgb`, `surface_rect`, `surface_text`.
- Read selection: `surface_poll`, compare raw event constants, `surface_pos`, `pair_first`, `pair_second`, row `rect_contains`.
- Keep event loop stable: use `sleep_ms` after a poll/render pass so an idle editor does not busy-spin.
- Navigate view: selected row index goes through `state_index_set`, `open_child`, `view_push`, `view_pop`, or `state_hash_set`.
- Separate edit and browse state: keep the edited boot block in `boot.editor.view`, the edit row in `boot.editor.index`, the browser root in `boot.browser.view`, and the selected insert token in `boot.browser.token` using `var_read` and `var_write`.
- Browse graph: render the selected browser hash and its first eight network children with `graph_children` and `child_at`; clicking a child restores `boot.browser.view`, sets `state_index_set`, calls `open_child`, then saves the new browser hash as both current browser view and selected token.
- Edit chain: build a `call` record around `boot.browser.token` with `record_pack_hash`, then use `records_insert`, `records_replace`, `records_delete`, and update `boot.editor.view` plus `state_hash_set`.
- Persist boot: call `save_boot` after the edited view hash is installed in state.
- Publish graph: call `publish_view` for discoverability.

## Confirmation

No boot-editor-specific instruction token is required. The remaining editor behavior is block-level composition over generic tokens: numbers, stack shuffling, bytes, records, rectangles, surface events, state, and persistence.
