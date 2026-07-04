typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ int r=dxgfx_mouse_wheel(); push(&r,4); cont(); }
