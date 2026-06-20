#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H h;
    cvm_pop(h);
    if (s) memcpy(s->cur_hash, h, 32);
    cnext();
}
