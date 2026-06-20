#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H key, out;
    u8 *d = 0;
    u32 len = 0;
    cvm_pop(key);
    cvm_zero(out);
    if (!net_uget(key, out)) {
        if (net_file(key, &d, &len)) { block_write(d, len, out); free(d); }
    }
    cvm_push(out);
    cnext();
}
