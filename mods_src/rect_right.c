#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,a,b,o; cvm_pop(r); cvm_zero(a); cvm_zero(b); memcpy(a,r,8); memcpy(b,r+16,8); cvm_u64_to_h(cvm_h_to_u64(a)+cvm_h_to_u64(b),o); cvm_push(o); cnext(); }
