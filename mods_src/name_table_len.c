#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void*, u32);
/* -> u32 count */
__declspec(dllexport) void run(void){
    name_load();
    push(&g_name_n, 4);
    cont();
}
