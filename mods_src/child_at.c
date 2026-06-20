#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H h, idx_h, out;
    u32 len = 0;
    cvm_pop(idx_h);
    cvm_pop(h);
    u8 *d = block_read(h, &len);
    cvm_zero(out);
    if (d && len >= 4) {
        u32 idx = (u32)cvm_h_to_u64(idx_h);
        u32 n = net_u32be(d);
        if (idx < n && 4 + idx * 40 + 32 <= len) memcpy(out, d + 4 + idx * 40, 32);
    }
    if (d) free(d);
    cvm_push(out);
    cnext();
}
