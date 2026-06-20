#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H h; cvm_pop(h); if(s)s->view_index=cvm_h_to_u64(h); cnext(); }
