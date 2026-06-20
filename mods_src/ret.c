#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s && s->ret_jb) longjmp(*s->ret_jb, 1);
}
