#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,o; cvm_pop(r); cvm_zero(o); memcpy(o,r+8,8); cvm_push(o); cnext(); }
