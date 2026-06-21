#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H o; cvm_u64_to_h(32,o); cvm_push(o); cnext(); }
