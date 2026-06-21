#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); cvm_u64_to_h(cvm_h_to_u64(a)>cvm_h_to_u64(b),o); cvm_push(o); cnext(); }
