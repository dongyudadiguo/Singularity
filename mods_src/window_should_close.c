#include <string.h>
typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *slot(u32 size);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { u32 r = dxgfx_window_should_close() ? 1u : 0u; memcpy(slot(4), &r, 4); cont(); }
