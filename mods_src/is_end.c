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
    // If the offset is out of bounds or points to 32 zero bytes
    if (!d || offset + 32 > len) {
        out[0] = 1;
    } else {
        int zero = 1;
        for (int i = 0; i < 32; i++) {
            if (d[offset + i] != 0) {
                zero = 0;
                break;
            }
        }
        if (zero) {
            out[0] = 1;
        }
    }
    if (d) free(d);
    cvm_push(out);
}
