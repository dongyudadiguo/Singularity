#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h1, h2;
    cvm_pop(h2);
    cvm_pop(h1);
    u32 len1 = 0, len2 = 0;
    u8 *d1 = block_read(h1, &len1);
    u8 *d2 = block_read(h2, &len2);
    H out;
    cvm_zero(out);
    if (d1 && d2 && len1 == len2 && memcmp(d1, d2, len1) == 0) {
        out[0] = 1;
    }
    if (d1) free(d1);
    if (d2) free(d2);
    cvm_push(out);
    cnext();
}
