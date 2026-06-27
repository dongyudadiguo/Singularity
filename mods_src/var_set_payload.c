#include "mod.h"

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = cvm_payload();
    if (cvm_payload_size() < 36) { cont(); return; }
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 size = *(u32*)(p + 32);
    cvm_var_set(id, size);
    cont();
}