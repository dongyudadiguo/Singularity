#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H x,y,o; cvm_u64_to_h(s?s->surface_x:0,x); cvm_u64_to_h(s?s->surface_y:0,y); cvm_zero(o); memcpy(o,x,8); memcpy(o+8,y,8); cvm_push(o); cnext(); }
