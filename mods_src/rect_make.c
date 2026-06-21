#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H x,y,w,h,o; cvm_pop(h); cvm_pop(w); cvm_pop(y); cvm_pop(x); cvm_zero(o); memcpy(o,x,8); memcpy(o+8,y,8); memcpy(o+16,w,8); memcpy(o+24,h,8); cvm_push(o); cnext(); }
