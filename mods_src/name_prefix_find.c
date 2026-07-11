#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
/* stack: query[256] -> token[32] (zero if none). Thin table scan; match is str_prefix_ci_us. */
__declspec(dllexport) void run(void){
    char *q = (char*)pop(256);
    H out; memset(out, 0, 32);
    name_load();
    if (q && q[0]) {
        for (u32 i = 0; i < g_name_n; i++) {
            if (str_prefix_ci_us(g_names[i].name, q)) {
                memcpy(out, g_names[i].token, 32);
                break;
            }
        }
    }
    push(out, 32);
    cont();
}
