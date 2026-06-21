#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H p,o; cvm_pop(p); cvm_zero(o); memcpy(o,p+8,8); cvm_push(o); cnext(); }
