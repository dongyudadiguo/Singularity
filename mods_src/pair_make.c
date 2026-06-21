#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); cvm_zero(o); memcpy(o,a,8); memcpy(o+8,b,8); cvm_push(o); cnext(); }
