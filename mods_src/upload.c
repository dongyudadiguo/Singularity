#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H h, out;
    u32 len = 0;
    cvm_pop(h);
    cvm_zero(out);
    u8 *d = block_read(h, &len);
    if (d) { net_upload(d, len, out); free(d); }
    cvm_push(out);
}
