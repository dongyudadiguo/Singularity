#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H off_h,o,s,l; cvm_pop(off_h); cvm_u64_to_h(cvm_h_to_u64(off_h),s); cvm_u64_to_h(32,l); cvm_zero(o); memcpy(o,s,8); memcpy(o+8,l,8); cvm_push(o); cnext(); }
