#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    CvmSurfaceContext *ctx = surface_context();
    H out;
    cvm_u64_to_h((s && ctx && s->surface_event == WM_CHAR) ? (u64)ctx->last_char : 0, out);
    cvm_push(out);
    cnext();
}
