#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H h, out;
    u32 len = 0;
    cvm_pop(h);
    u8 *d = block_read(h, &len);
    cvm_zero(out);
    if (d && len >= 4) cvm_u64_to_h(net_u32be(d), out);
    if (d) free(d);
    cvm_push(out);
    cnext();
}
