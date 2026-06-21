#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s){s->surface_event=0;s->surface_x=0;s->surface_y=0;} cnext(); }
