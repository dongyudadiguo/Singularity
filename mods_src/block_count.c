#include "block_chain.h"
__declspec(dllexport) void run(void) {
    H h, out;
    u32 len = 0;
    cvm_pop(h);
    u8 *d = block_read(h, &len);
    cvm_u64_to_h(d ? bc_count(d, len) : 0, out);
    if (d) free(d);
    cvm_push(out);
    cnext();
}
