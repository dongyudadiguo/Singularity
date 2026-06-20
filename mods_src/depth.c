#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H d;
    cvm_zero(d);
    if (s) {
        cvm_u64_to_h((u64)s->sp, d);
    }
    cvm_push(d);
    cnext();
}
