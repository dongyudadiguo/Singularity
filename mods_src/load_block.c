#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    H h;
    u32 len = 0;
    cvm_pop(h);
    u8 *d = block_read(h, &len);
    if (d) free(d);
    cvm_push(h);
    cnext();
}
