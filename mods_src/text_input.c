typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ u32 cp=0; dxgfx_text_input(&cp); push(&cp,4); cont(); }
