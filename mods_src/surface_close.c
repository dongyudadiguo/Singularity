#include "surface_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); surface_context_reset(); if(s&&s->surface_hwnd){DestroyWindow(s->surface_hwnd); s->surface_hwnd=0;} cnext(); }
