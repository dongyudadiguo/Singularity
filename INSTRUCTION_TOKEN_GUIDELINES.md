# Instruction Token Guidelines

This file defines how new instruction tokens must be designed for this VM.

## Core Rule

Instruction token semantics must grow from low-level, general, platform-neutral primitives toward higher-level, specialized behavior.

Do not jump levels. Do not add a specialized instruction if it can be easily composed from existing lower-level instructions.

Token blocks are different: token blocks may freely compose behavior at any level, including application-specific behavior such as a graphical boot editor.

## Layer Order

New instruction tokens should fit into one of these layers.

1. L0 data movement and values
2. L0 numeric, boolean, byte, hash, and stack primitives
3. L1 generic structures: pair, range, rect, record, block, bytes
4. L2 generic VM state and chain execution controls
5. L3 narrow platform edges: surface, input, persistence, network, file
6. L4 application behavior: must be token blocks, not new instruction tokens

If a proposed token belongs to L4, reject it as an instruction token and implement it as a token block instead.

## Granularity

Instruction tokens must be small.

Good examples:

- `payload_u64_le`: read a u64 from current payload.
- `rect_contains`: test whether a point is inside a rect.
- `records_insert`: insert one record into a record chain.
- `surface_text`: draw bytes at a position.

Bad examples:

- `show_menu`: application-specific rendering.
- `edit_insert_call`: editor-specific mutation policy.
- `boot_editor_loop`: whole program behavior.
- `draw_token_list`: application-specific layout and rendering.

## Composition Requirement

Before adding a token, check whether the behavior can already be composed easily from existing tokens.

Add the token only if at least one condition is true:

- The existing composition requires excessive stack juggling or repeated byte manipulation.
- The behavior is a broadly useful primitive across many token blocks.
- The token closes a low-level semantic gap in an otherwise uniform layer.
- The token exposes a narrow platform boundary that cannot be expressed inside the VM.

Do not add the token if the reason is only convenience for one application.

## Uniform Distribution

Instruction tokens should not create semantic cliffs.

Avoid a distribution where one area has many high-level helpers while neighboring lower-level operations are missing.

Prefer filling lower-level gaps first:

- Add `payload_hash32` before adding a token that inserts a specific call.
- Add `rect_contains` before adding button-click commands.
- Add `surface_round_rect` before requiring applications to fake rounded corners with layout-specific block piles.
- Add `graph_child_at` before requiring every browser row to fetch and decode a full child list.
- Add `over`, `rot`, `nip`, and `tuck` before adding large control macros.
- Add `records_insert`, `records_replace`, and `records_delete` before adding editor commands.

## Platform Boundaries

Platform-specific instructions are allowed only as narrow edges.

Allowed examples:

- `surface_open`
- `surface_poll`
- `surface_text`
- `surface_round_rect`
- `sleep_ms`
- `load_boot`
- `save_boot`

Disallowed examples:

- `surface_draw_editor`
- `surface_menu_select`
- `save_editor_boot`

The instruction may expose an operation the platform must perform, but it must not encode application policy.

## Token Blocks

Token blocks have no such semantic-layer restriction.

Use token blocks for:

- Boot programs
- Graphical editors
- Menus
- Layout policy
- Editing workflows
- Persistence workflows
- Application-specific state machines

A token block may compose many tokens into high-level behavior. That is the intended place for specialized logic.

## Boot Program Composition Rule

The first-start boot program must be easy to build as token blocks from existing instruction tokens.

When it is not easy, do not work around the problem by moving boot behavior into host code or adding a boot-specific instruction. First identify the missing low-level capability, then add the smallest generic token or VM semantic needed to make the composition straightforward.

Acceptable fixes:

- Fix generic chain control such as loop/restart behavior.
- Fix generic call semantics so token blocks can be safely split into smaller blocks.
- Add a reusable L0-L3 primitive that unrelated token programs can also use.

Unacceptable fixes:

- Add `boot_editor_loop`, `draw_boot_editor`, or other L4 application instructions.
- Hide per-frame boot behavior in `vm.c` or `boot_run.c` when it should be visible as token composition.
- Depend on repeated host-level restart of the boot program as the event loop.

## Review Checklist

Before accepting a new instruction token, answer these questions.

- Which layer does it belong to?
- Is it lower-level than the behavior that will use it?
- Can it be composed easily from existing tokens?
- Does it fill a general semantic gap?
- Is the name generic and application-neutral?
- Does it avoid embedding UI, editor, boot, or menu policy?
- Is the operation small enough to be reused in unrelated token blocks?
- Does it avoid depending on host-level restart loops for program behavior?

If any answer fails, do not add the instruction token. Build a token block instead.

## Current Boot Editor Boundary

The first boot graphical self-editor must be built as token blocks from existing primitives.

Its core dependencies are generic tokens only:

- Payload immediates: `payload_byte`, `payload_u64_le`, `payload_hash32`
- Stack: `dup`, `swap`, `over`, `rot`, `nip`, `tuck`
- Numeric and boolean: `lt`, `gt`, `le`, `ge`, `eq`, `ne`, `and`, `or`, `not`
- Records: `record_pack`, `record_pack_hash`, `records_insert`, `records_replace`, `records_delete`
- Rectangles and color: `rect_make`, `rect_contains`, `color_rgb`
- Surface: `surface_open`, `surface_clear`, `surface_rect`, `surface_frame`, `surface_round_rect`, `surface_round_frame`, `surface_text`, `surface_poll`, `surface_pos`, `surface_event_clear`
- Network graph: `graph_children`, `graph_child_at`, `open_child`, `child_at`
- Loop pacing: `sleep_ms`
- State and persistence: `state_hash_get`, `state_hash_set`, `state_index_get`, `state_index_set`, `load_boot`, `save_boot`, `publish_view`

Do not add editor-specific instruction tokens unless this boundary is proven insufficient by an actual failed composition attempt.
