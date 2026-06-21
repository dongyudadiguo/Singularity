#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; cvm_u64_to_h(s?s->view_index:0,o); cvm_push(o); cnext(); }
