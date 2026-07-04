typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../editorcore.h"
__declspec(dllexport) void run(void){ u32 r=(u32)ec_should_halt(); push(&r,4); cont(); }
