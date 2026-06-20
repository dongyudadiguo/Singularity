#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h;
    cvm_pop(h);
    u32 len = 0;
    u8 *d = block_read(h, &len);
    H out;
    cvm_u64_to_h(len, out);
    cvm_push(out);
    if (d) free(d);
    cnext();
}
