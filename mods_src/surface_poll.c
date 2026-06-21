#include "surface_ops.h"
__declspec(dllexport) void run(void) { MSG msg; while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageA(&msg);} CvmState*s=cvm_state(); H o; cvm_u64_to_h(s?s->surface_event:0,o); cvm_push(o); cnext(); }
