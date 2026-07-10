typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
__declspec(dllexport) void run(void){u32 v=*(u32*)pop(4);if(v)v--;push(&v,4);cont();}
