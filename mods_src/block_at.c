#include "block_chain.h"
__declspec(dllexport) void run(void) {
    H h, idx_h, out;
    u32 len = 0, s = 0, e = 0;
    cvm_pop(idx_h);
    cvm_pop(h);
    cvm_zero(out);
    u8 *d = block_read(h, &len);
    if (d && bc_range(d, len, (u32)cvm_h_to_u64(idx_h), &s, &e)) block_write(d + s, e - s, out);
    if (d) free(d);
    cvm_push(out);
}
