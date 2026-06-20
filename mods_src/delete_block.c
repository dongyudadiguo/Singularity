#include "block_chain.h"
__declspec(dllexport) void run(void) {
    H chain_h, idx_h, out;
    u32 len = 0, s = 0, e = 0;
    cvm_pop(idx_h);
    cvm_pop(chain_h);
    u8 *d = block_read(chain_h, &len);
    cvm_zero(out);
    if (d) {
        if (bc_range(d, len, (u32)cvm_h_to_u64(idx_h), &s, &e)) bc_save_edit(d, s, d + e, len - e, 0, 0, out);
        else memcpy(out, chain_h, 32);
    }
    if (d) free(d);
    cvm_push(out);
    cnext();
}
