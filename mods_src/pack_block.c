#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H tok_h, pay_h;
    cvm_pop(pay_h);
    cvm_pop(tok_h);
    u32 len = 0;
    u8 *pay = block_read(pay_h, &len);
    u32 sp = len + 4;
    u32 total = 32 + 4 + len;
    u8 *buf = malloc(total + 1);
    H out_hash;
    cvm_zero(out_hash);
    if (buf) {
        memcpy(buf, tok_h, 32);
        buf[32] = (u8)(sp);
        buf[33] = (u8)(sp >> 8);
        buf[34] = (u8)(sp >> 16);
        buf[35] = (u8)(sp >> 24);
        if (pay && len > 0) {
            memcpy(buf + 36, pay, len);
        }
        block_write(buf, total, out_hash);
        free(buf);
    }
    if (pay) free(pay);
    cvm_push(out_hash);
}
