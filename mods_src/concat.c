#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h1, h2;
    cvm_pop(h2);
    cvm_pop(h1);
    u32 len1 = 0, len2 = 0;
    u8 *d1 = block_read(h1, &len1);
    u8 *d2 = block_read(h2, &len2);
    H out_hash;
    cvm_zero(out_hash);
    if (d1 || d2) {
        u32 total = len1 + len2;
        u8 *tmp = malloc(total + 1);
        if (tmp) {
            if (d1) memcpy(tmp, d1, len1);
            if (d2) memcpy(tmp + len1, d2, len2);
            block_write(tmp, total, out_hash);
            free(tmp);
        }
    }
    if (d1) free(d1);
    if (d2) free(d2);
    cvm_push(out_hash);
}
