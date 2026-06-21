#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,d,o,l; cvm_pop(d); cvm_pop(r); cvm_zero(l); memcpy(l,r+8,8); cvm_u64_to_h(cvm_h_to_u64(l)+cvm_h_to_u64(d),l); cvm_zero(o); memcpy(o,r,8); memcpy(o+8,l,8); cvm_push(o); cnext(); }
