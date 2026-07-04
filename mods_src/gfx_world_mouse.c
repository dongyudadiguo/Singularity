typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ float xy[2]={0,0}; dxgfx_world_mouse(xy); push(xy,8); cont(); }
