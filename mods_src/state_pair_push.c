#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H h,i; cvm_zero(h); if(s)memcpy(h,s->view_hash,32); cvm_u64_to_h(s?s->view_index:0,i); cvm_push(h); cvm_push(i); cnext(); }
