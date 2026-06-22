#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"

__declspec(dllexport) void run(void) {
    H h, out;
    cvm_pop(h);
    u32 len = 0;
    u8 *d = block_read(h, &len);
    cvm_zero(out);
    if (d) {
        u32 new_len = 0;
        if (len) {
            u32 i = len;
            while (i > 0 && (d[i - 1] & 0xc0) == 0x80) i--;
            new_len = (i == len) ? len - 1 : (i ? i - 1 : 0);
        }
        block_write(d, new_len, out);
        free(d);
    } else {
        block_write((u8*)"", 0, out);
    }
    cvm_push(out);
    cnext();
}
