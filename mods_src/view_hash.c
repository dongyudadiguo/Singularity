#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H out; cvm_zero(out); if(s)memcpy(out,s->view_hash,32); cvm_push(out); cnext(); }
