#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s && s->sp > 0 && s->sp < CVM_STACK_CAP) {
        memcpy(s->stack[s->sp], s->stack[s->sp - 1], 32);
        s->sp++;
    }
}
