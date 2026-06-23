#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    CvmSurfaceContext *ctx = surface_context();
    H out;
    u64 dir = 0;
    if (ctx && ctx->wheel_delta > 0) dir = 1;
    else if (ctx && ctx->wheel_delta < 0) dir = 2;
    cvm_u64_to_h(dir, out);
    cvm_push(out);
    cnext();
}
