#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H o; cvm_zero(o); cvm_push(o); cnext(); }
