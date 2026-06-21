#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H start,len,o; cvm_pop(len); cvm_pop(start); cvm_zero(o); memcpy(o,start,8); memcpy(o+8,len,8); cvm_push(o); cnext(); }
