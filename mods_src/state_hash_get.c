#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; cvm_zero(o); if(s)memcpy(o,s->view_hash,32); cvm_push(o); cnext(); }
