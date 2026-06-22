#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"

__declspec(dllexport) void run(void) {
    H cp_h, out;
    cvm_pop(cp_h);
    u64 cp = cvm_h_to_u64(cp_h);
    u8 buf[4];
    u32 len = 0;
    cvm_zero(out);
    if (cp <= 0x7f) {
        buf[len++] = (u8)cp;
    } else if (cp <= 0x7ff) {
        buf[len++] = (u8)(0xc0 | (cp >> 6));
        buf[len++] = (u8)(0x80 | (cp & 0x3f));
    } else if (cp <= 0xffff && !(cp >= 0xd800 && cp <= 0xdfff)) {
        buf[len++] = (u8)(0xe0 | (cp >> 12));
        buf[len++] = (u8)(0x80 | ((cp >> 6) & 0x3f));
        buf[len++] = (u8)(0x80 | (cp & 0x3f));
    } else if (cp <= 0x10ffff) {
        buf[len++] = (u8)(0xf0 | (cp >> 18));
        buf[len++] = (u8)(0x80 | ((cp >> 12) & 0x3f));
        buf[len++] = (u8)(0x80 | ((cp >> 6) & 0x3f));
        buf[len++] = (u8)(0x80 | (cp & 0x3f));
    }
    block_write(buf, len, out);
    cvm_push(out);
    cnext();
}
