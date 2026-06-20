#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h, offset_h;
    cvm_pop(offset_h);
    cvm_pop(h);
    u64 offset = cvm_h_to_u64(offset_h);
    u32 len = 0;
    u8 *d = block_read(h, &len);
    H out;
    cvm_zero(out);
    if (d && offset + 36 <= len) {
        u8 *p = d + offset;
        u32 sp = (u32)p[32] | ((u32)p[33] << 8) | ((u32)p[34] << 16) | ((u32)p[35] << 24);
        u64 next_offset = offset + 32 + sp;
        cvm_u64_to_h(next_offset, out);
    }
    if (d) free(d);
    cvm_push(out);
}
