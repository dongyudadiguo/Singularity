#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,d,o,s,l; cvm_pop(d); cvm_pop(r); cvm_zero(s); cvm_zero(l); memcpy(s,r,8); memcpy(l,r+8,8); cvm_u64_to_h(cvm_h_to_u64(s)+cvm_h_to_u64(d),s); cvm_zero(o); memcpy(o,s,8); memcpy(o+8,l,8); cvm_push(o); cnext(); }
