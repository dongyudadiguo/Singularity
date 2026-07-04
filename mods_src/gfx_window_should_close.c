typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ u32 r=(u32)dxgfx_window_should_close(); push(&r,4); cont(); }
