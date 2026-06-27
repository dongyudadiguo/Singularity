#include "mod.h"

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = pop(32);
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 vsize;
    if (!cvm_var_get(id, &vsize)) { cont(); return; }
    u8 *data = pop(vsize);
    cvm_var_write(id, data, vsize);
    cont();
}
