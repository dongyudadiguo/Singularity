#include "../cvm_state.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    block(0, 0);
    if (s) cvm_push(s->cur_hash);
}
