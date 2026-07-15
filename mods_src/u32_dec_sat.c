#include <string.h>
typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *from(u32); extern __declspec(dllimport) void *slot(u32);
__declspec(dllexport) void run(void){u32 v=*(u32*)from(4);if(v)v--;memcpy(slot(4), &v, 4);cont();}
