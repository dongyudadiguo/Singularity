#include "net_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H key,out; cvm_pop(key); cvm_zero(out); if(s&&net_uset(key,s->view_hash))out[0]=1; cvm_push(out); cnext(); }
