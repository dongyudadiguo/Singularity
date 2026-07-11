#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
/* stack: u32 index -> token[32] + name[96] (zeros if OOB) */
__declspec(dllexport) void run(void){
    u32 idx = *(u32*)pop(4);
    H tok; char name[96];
    memset(tok, 0, 32); memset(name, 0, 96);
    name_load();
    if (idx < g_name_n) {
        memcpy(tok, g_names[idx].token, 32);
        memcpy(name, g_names[idx].name, 95);
    }
    push(tok, 32);
    push(name, 96);
    cont();
}
