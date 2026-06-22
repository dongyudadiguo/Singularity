#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H cond;
    cvm_pop(cond);
    if (cvm_truth(cond)) {
        CvmState *s = cvm_state();
        if (s) ccont(s->chain_start);
    } else {
        cnext();
    }
}
