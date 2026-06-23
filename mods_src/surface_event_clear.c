#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); CvmSurfaceContext*ctx=surface_context(); if(s){s->surface_event=0;s->surface_x=0;s->surface_y=0;} if(ctx){ctx->last_char=0;ctx->wheel_delta=0;} cnext(); }
