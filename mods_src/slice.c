#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h, out_offset, out_length;
    cvm_pop(out_length);
    cvm_pop(out_offset);
    cvm_pop(h);
    u64 offset = cvm_h_to_u64(out_offset);
    u64 length = cvm_h_to_u64(out_length);
    u32 blen = 0;
    u8 *d = block_read(h, &blen);
    H out_hash;
    cvm_zero(out_hash);
    if (d) {
        if (offset < blen) {
            if (offset + length > blen) {
                length = blen - offset;
            }
            if (length > 0) {
                block_write(d + offset, (u32)length, out_hash);
            }
        }
        free(d);
    }
    cvm_push(out_hash);
}
