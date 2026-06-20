#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s) s->sp = 0;
}
