# First Boot Playground Plan

## Goal

Improve the first-start boot program from a hash/record editor into a direct exploration loop:

Browse published blocks, play with a selected block or token, fold useful discoveries into the boot block, and publish the resulting block without leaving the first-start UI.

## Decisions

- Keep the direct boot-editor model selected by the user: edits act on the current boot block, not a separate sandbox/workbench.
- Ignore runtime isolation for now: `Play selected` may execute inside the editor VM process and may mutate state or surface contents.
- Default the right-side browser to the public root graph so the first thing users see is published content.
- Keep a clear Catalog shortcut so tokens and starter fragments remain discoverable without making Catalog the default view.
- Publishing generally publishes blocks. If the selected item is a raw token, automatically wrap it into a single-record block before publishing.
- Deploy after implementation: dry-run the builder, then upload blocks/edges and set the current `id.bin` user boot.

## Current Context

- `boot_editor_builder.go` generates the first-start graphical self-editor as token blocks.
- Existing flow is mostly low-level: right side browses `catalog_root`; left side edits `boot.editor.view`; buttons insert/replace/delete and publish/save boot.
- Existing generic tokens already cover the required low-level operations:
  - Execute selected block: `call_stack`.
  - Detect block-like hash: `records_valid`.
  - Wrap raw token as block: `record_pack_empty`, `records_empty`, `records_insert`.
  - Publish current state hash to root: `publish_view`.
  - Link graph edges if needed: `graph_link` / `graph_link_root`.
- `vm.c` starts by walking from root to the first child with a DLL implementation. `boot_editor_builder.go` currently keeps `boot_run` as root child and votes it. Do not break this invariant.

## Implementation Tasks

1. Rework boot editor initialization in `boot_editor_builder.go`.

- Initialize `boot.editor.view` from `state_hash_get` as today.
- Initialize `boot.browser.view` to the zero/root hash instead of `catalog_root`.
- Initialize `boot.browser.token` to zero or the currently opened browser item.
- Keep `boot.editor.index` and `boot.editor.dirty` setup.
- Preserve the single token-block frame loop; do not move UI/event-loop behavior into host C code.

2. Add browser shortcuts.

- Add a `browser_root` block that sets `boot.browser.view` and `boot.browser.token` to zero/root.
- Add a `browser_catalog` block that sets `boot.browser.view` and `boot.browser.token` to the generated `catalog_root` block.
- Keep `browser_back` and child-row browsing behavior.
- Add visible `Root` and `Catalog` buttons in the browser action area.

3. Add selected-item wrapping helpers.

- Add a helper block that builds a playable/publishable block from `boot.browser.token` when the selected item is not already a valid record chain:
  - `records_empty`
  - `payload_u64_le 0`
  - `var_read boot.browser.token`
  - `record_pack_empty`
  - `records_insert`
- Add branch blocks for valid-block and raw-token cases using `records_valid` and `not` with `call_cond_static`.
- When wrapping for publish, store the wrapper hash back into `boot.browser.token` so the UI shows the actual published block hash afterward.

4. Add `Play selected`.

- If `records_valid(boot.browser.token)` is true, run it with `call_stack`.
- If false, wrap the raw token into a single-record block and run that wrapper with `call_stack`.
- Pop the `call_stack` return hash afterward to avoid unnecessary stack growth.
- Mark the editor dirty after play so the normal draw pass redraws the UI.
- Do not attempt sandboxing in this pass.

5. Add easy accumulation into the left boot block.

- Keep existing precise edit actions if space allows: insert call, insert raw token, replace call, replace raw token, delete.
- Add a new smart `Add selected` / `Append selected` action for the main flow:
  - If selected is already a valid block, append a `call` record pointing at it.
  - If selected is a raw token, append the raw token as an empty-payload record.
  - Use `records_count` to append at the end of `boot.editor.view`.
  - Update `boot.editor.view`, `state_hash_set`, `publish_view`, and `save_boot` consistently with the current editor mutation behavior.
- Keep row clicks as the explicit edit index for advanced insert/replace operations.

6. Add block publishing actions.

- Keep or rename the existing left-side publish action as `Publish edit` / `Publish boot`:
  - `var_read boot.editor.view`
  - `state_hash_set`
  - `publish_view`
  - `save_boot`
- Add `Publish selected`:
  - If selected is a valid block, set it as state hash and call `publish_view`.
  - If selected is a raw token, wrap it into a single-record block, store that wrapper as `boot.browser.token`, set it as state hash, then call `publish_view`.
- Publishing selected should not save it as boot unless it is also explicitly added to or published as the left-side edit.

7. Update generated catalog/starter content.

- Keep current token catalog categories.
- Add a few small generated starter fragment blocks under the catalog, using only existing generic tokens. Examples:
  - Existing rounded rectangle fragment.
  - Text/badge fragment.
  - Simple color/shape fragment.
- These fragments should be normal blocks, browsable, playable, addable, and publishable.
- Do not add application-specific instruction tokens.

8. Update draw/layout.

- Retitle the UI around the actual flow, for example `Browse -> Play -> Add -> Publish`.
- Show the current browser root/public state, selected hash, edit hash, and edit index/count.
- Prefer decimal display for indices/counts using existing numeric-to-bytes tokens if practical.
- Keep the UI simple enough for 1280-wide usage; increasing the surface height to 720 is acceptable if needed for controls.
- Keep 8 visible rows unless implementation can increase rows without making event dispatch brittle.

9. Preserve root boot behavior during edge/deploy generation.

- Continue adding and voting `root -> boot_run` in `main()` deployment.
- If adding `root -> catalog_root` for discoverability, do not vote it above `boot_run`.
- Prefer a fixed Catalog button over relying on Catalog being high in public root order.
- Do not change `vm.c` root walking as part of this feature.

10. Update design documentation.

- Update `BOOT_EDITOR_COMPOSITION.md` to reflect the new first-start flow:
  - Public root browsing by default.
  - Catalog shortcut.
  - Play selected via `call_stack`.
  - Raw token wrapping before play/publish when a block is required.
  - Direct publish/save behavior.
- Only update `INSTRUCTION_TOKEN_GUIDELINES.md` if implementation discovers a real generic token gap.

## Fallback Rules

- If a behavior cannot be composed cleanly from existing tokens, do not add a boot/editor-specific instruction token.
- If a new token is needed, it must be a small reusable L0-L3 primitive and the implementation agent should stop and present options before adding it.
- If `records_valid` proves unreliable for distinguishing selected blocks from raw tokens, use explicit UI actions (`Play block`, `Play token`, `Publish block`, `Publish token as block`) rather than adding host-side special cases.

## Validation

1. Build/generate without upload:

```powershell
go run .\boot_editor_builder.go -dry-run
```

2. Confirm the dry-run output includes new named blocks for root/catalog shortcuts, play selected, publish selected, wrapping selected token, and smart add/append.

3. Verify all referenced token names exist in `mods_map.txt`.

4. Run the generated boot editor locally if practical:

```powershell
.\vm.exe
```

5. Manual UI checks:

- First screen opens with the browser on public root.
- Catalog button opens token/starter catalog.
- Child rows can browse and select published items.
- Play selected runs a selected block and also handles a raw token by wrapping it.
- Add/Append selected updates the left boot block.
- Publish selected publishes a block; raw token selection becomes a wrapper block before publish.
- Publish edit saves and publishes the left edited boot block.
- Back navigation still works.

6. Deploy after validation:

```powershell
go run .\boot_editor_builder.go
```

7. After deployment, restart `vm.exe` and confirm the new first-start boot editor appears for the current `id.bin` user.

## Risks Accepted By Scope

- Inline play can mutate editor state, draw over the same surface, or hang if the played block loops forever.
- Edit mutations may continue to save/publish immediately as the current editor does.
- Public root is also used for VM startup discovery, so deployment must keep `boot_run` voted as the top startup child.

## Out Of Scope

- Separate sandbox process for Play.
- Authentication or moderation changes for publishing.
- A full text/name metadata system for blocks.
- Replacing `vm.c` startup resolution.
