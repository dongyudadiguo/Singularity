#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h;
    cvm_pop(h);
    u32 len = 0;
    u8 *d = block_read(h, &len);
    H out;
    cvm_zero(out);
    if (d && len >= 36) {
        u32 sp = (u32)d[32] | ((u32)d[33] << 8) | ((u32)d[34] << 16) | ((u32)d[35] << 24);
        cvm_u64_to_h(sp, out);
    }
    if (d) free(d);
    cvm_push(out);
}
