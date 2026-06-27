#include "mod.h"

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = pop(32);
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 size;
    u8 *data = cvm_var_get(id, &size);
    if (data) push(data, size);
    cont();
}
