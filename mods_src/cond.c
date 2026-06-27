#include "mod.h"

__declspec(dllexport) void run(void) {
    H h;
    int ok = mod_bool(pop(4));
    u8 *p = pop(32);
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (ok) cvm_exec(h);
    else cont();
}
