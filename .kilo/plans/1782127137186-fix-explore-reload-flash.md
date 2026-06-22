# Fix Explore Panel Reload & Screen Flash

## Problem

Opening the program window causes explore panel entries to continuously reload and the entire view to flash.

## Root Causes (3 layers)

1. **Forced per-frame redraw** (`boot_editor_builder.go:1635-1636`): Frame loop set `boot.editor.dirty=1` unconditionally every ~33ms, causing full `surface_clear` + repaint of the entire Home UI and Explore panel every frame.

2. **Per-frame network calls during draw** (`boot_editor_builder.go:1563-1573`): Explore panel rows called `graph_child_at` inside the draw block. Each `graph_child_at` makes a remote `OP_CHILDREN` request. With 6 rows × 30fps = ~180 network calls/sec, plus network latency between clear and repaint = visible flash + entries appearing to "reload."

3. **Recursive continuation stack growth** (`continue.h`): The old continuation model had each token `cnext()` recursively call `ccont()` which invoked the next token's DLL. Frame loops using `again_cond` never unwound, causing stack overflow and process exit after a few seconds. Failed attempt to fix with cross-DLL `longjmp` caused access violation (exit code `-1073741819`).

4. **Old `boot_run` selected by root graph** (`vm.c:56-64`): `walk()` picks the highest-score child of public root. Historical votes mean the old `boot_run` hash still ranks first. The old DLL uses recursive continuation, incompatible with new token DLLs that use return-loop model — results in clean exit (code 0) after one token.

## What's Already Done (source files)

| File | Changes |
|------|---------|
| `boot_editor_builder.go` | Removed per-frame forced dirty; explore draws from cached `graph_children`; `surface_event_clear` after open; browser cache refresh on view changes; stage-only idle redraw |
| `surface_ops.h` | `WM_ERASEBKGND` returns 1; `hbrBackground=0`; `WM_PAINT`/`WM_SIZE` set `surface_event`; `surface_hwnd()` validates `IsWindow` |
| `surface_open.c` | Clears stale HWND and `surface_event` before create; validates `IsWindow` after |
| `surface_close.c` | Validates `IsWindow` before `DestroyWindow`; clears HWND/event |
| `surface_is_open.c` | Validates `IsWindow` |
| `continue.h` | Return-loop model: `cnext`/`ccont` set `next_off` and return; `cbegin` loops calling `cexec` |
| `cvm_state.h` | Added `next_off` field; removed `cont_jb` |
| `ret.c` | Sets `next_off = CNOFF` |
| `mods_map.txt` + `mods/*.dll` | All ~284 token DLLs rebuilt to new continuation ABI (2 passes) |

Server state: Latest boot entry block (`1f79b93c...`) and all blocks uploaded; user's `CVM_BOOT` points to it.

## Remaining Fix: VM Entry Point

### Problem
`vm.c:main()` calls `boot()` → `walk()`, which picks the first child of public root (`0x00...00`) by server vote score. The old `boot_run` DLL still has the highest score. Even though server has the correct `CVM_BOOT` key-value, `vm.c` never reads it — it always goes through the public root graph.

### Solution
Modify `vm.c` to skip the public root graph walk. Instead:
1. Connect to server (existing `boot()` infrastructure)
2. Fetch user's `CVM_BOOT` key-value directly (new `net_uget`-style function)
3. `block_read` + locate the boot block body locally or from network
4. `cbegin` the boot block directly

### Files to change
- `mods_src/vm.c` — or create a new `boot_entry.c` module that replaces the old `boot_run.c` logic. Simpler: modify `vm.c` to bypass graph walk entirely.

### Validation
1. Rebuild `vm.c` → `vm.exe`
2. Launch `vm.exe`, wait 10 seconds
3. Verify process is still **running** (not exited)
4. Check exit code is not 0 (window stays open until user closes)

### Edge cases / risks
- `id.bin` must exist and contain a valid user identity (already the case since builder uploads succeeded)
- Network must be reachable at `118.25.42.70:9000` (already confirmed)
- Stale shared memory (`CVM_State` session) from previous crash — handled by `surface_open` clearing stale HWND/event
- `CVM_VAR_CAP` (64) vs variable count (42) — safe
