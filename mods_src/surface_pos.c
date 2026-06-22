#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); CvmSurfaceContext*ctx=surface_context(); H x,y,o; LONG tx=ctx?ctx->tx:0,ty=ctx?ctx->ty:0; cvm_u64_to_h(s?(u64)((LONG)s->surface_x-tx):0,x); cvm_u64_to_h(s?(u64)((LONG)s->surface_y-ty):0,y); cvm_zero(o); memcpy(o,x,8); memcpy(o+8,y,8); cvm_push(o); cnext(); }
