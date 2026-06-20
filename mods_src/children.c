#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H parent, out;
    u8 *d = 0;
    u32 len = 0;
    cvm_pop(parent);
    cvm_zero(out);
    if (net_children(parent, &d, &len)) { block_write(d, len, out); free(d); }
    cvm_push(out);
}
