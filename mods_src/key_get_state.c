#include "net_ops.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H key,out; cvm_pop(key); cvm_zero(out); net_uget(key,out); if(s)memcpy(s->view_hash,out,32); cvm_push(out); cnext(); }
