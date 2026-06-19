#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s) s->ret = 1;
}
