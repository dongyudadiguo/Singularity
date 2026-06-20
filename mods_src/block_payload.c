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
        u32 pl = sp > 4 ? sp - 4 : 0;
        if (pl > 0 && len >= 36 + pl) {
            block_write(d + 36, pl, out);
        }
    }
    if (d) free(d);
    cvm_push(out);
}
