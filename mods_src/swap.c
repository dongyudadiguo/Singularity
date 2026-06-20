#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s && s->sp >= 2) {
        H tmp;
        memcpy(tmp, s->stack[s->sp - 1], 32);
        memcpy(s->stack[s->sp - 1], s->stack[s->sp - 2], 32);
        memcpy(s->stack[s->sp - 2], tmp, 32);
    }
    cnext();
}
