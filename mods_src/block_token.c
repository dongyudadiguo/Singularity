#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h;
    cvm_pop(h);
    u32 len = 0;
    u8 *d = block_read(h, &len);
    H tok;
    cvm_zero(tok);
    if (d && len >= 32) {
        memcpy(tok, d, 32);
    }
    if (d) free(d);
    cvm_push(tok);
}
