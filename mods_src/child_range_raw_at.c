#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H idx_h,o,s,l; cvm_pop(idx_h); cvm_u64_to_h(4+cvm_h_to_u64(idx_h)*40,s); cvm_u64_to_h(40,l); cvm_zero(o); memcpy(o,s,8); memcpy(o+8,l,8); cvm_push(o); cnext(); }
