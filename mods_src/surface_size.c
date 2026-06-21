#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H x,y,w,h,o; cvm_zero(x); cvm_zero(y); cvm_u64_to_h(s?s->surface_w:0,w); cvm_u64_to_h(s?s->surface_h:0,h); cvm_zero(o); memcpy(o,x,8); memcpy(o+8,y,8); memcpy(o+16,w,8); memcpy(o+24,h,8); cvm_push(o); cnext(); }
