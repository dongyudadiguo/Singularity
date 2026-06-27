#include "mod.h"

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = cvm_payload();
    if (cvm_payload_size() < 32) { cont(); return; }
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 vsize;
    cvm_var_get(id, &vsize);
    u8 *data = pop();
    cvm_var_write(id, data, vsize);
    cont();
}