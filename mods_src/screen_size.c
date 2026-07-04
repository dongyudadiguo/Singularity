typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void *p, u32 size);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { int s[2] = {0,0}; dxgfx_screen_size(s); push(s, sizeof(s)); cont(); }
