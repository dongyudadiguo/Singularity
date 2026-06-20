#include "io_parse.h"
__declspec(dllexport) void run(void) { H h,out; cvm_pop(h); cvm_zero(out); if(!cvm_truth(h)) out[0]=1; cvm_push(out); cnext(); }
