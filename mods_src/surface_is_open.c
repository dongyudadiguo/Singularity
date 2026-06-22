#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H out;
    cvm_u64_to_h((s && s->surface_hwnd) ? 1 : 0, out);
    cvm_push(out);
    cnext();
}
