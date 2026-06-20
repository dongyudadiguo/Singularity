#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H out; cvm_zero(out); if(s)cvm_u64_to_h(s->view_index,out); cvm_push(out); cnext(); }
