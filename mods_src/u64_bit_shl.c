#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); u64 n=cvm_h_to_u64(b); cvm_u64_to_h(n<64?cvm_h_to_u64(a)<<n:0,o); cvm_push(o); cnext(); }
