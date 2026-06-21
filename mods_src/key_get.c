#include "net_ops.h"
__declspec(dllexport) void run(void) { H key,out; cvm_pop(key); cvm_zero(out); net_uget(key,out); cvm_push(out); cnext(); }
