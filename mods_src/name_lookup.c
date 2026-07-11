#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
/* stack: token[32] -> name[96] (hex4 fallback if missing; empty if zero token) */
__declspec(dllexport) void run(void){
    H token; memcpy(token, pop(32), 32);
    char out[96]; memset(out, 0, sizeof(out));
    int nz = 0; for (int i = 0; i < 32; i++) nz |= token[i];
    if (nz) {
        name_load();
        for (u32 i = 0; i < g_name_n; i++) {
            if (!memcmp(g_names[i].token, token, 32)) {
                strncpy(out, g_names[i].name, 95);
                break;
            }
        }
        if (!out[0])
            snprintf(out, sizeof(out), "%02x%02x%02x%02x", token[0], token[1], token[2], token[3]);
    }
    push(out, sizeof(out));
    cont();
}
