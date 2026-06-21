#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o,x,y,bx,by; cvm_pop(b); cvm_pop(a); cvm_zero(x); cvm_zero(y); cvm_zero(bx); cvm_zero(by); memcpy(x,a,8); memcpy(y,a+8,8); memcpy(bx,b,8); memcpy(by,b+8,8); cvm_u64_to_h(cvm_h_to_u64(x)+cvm_h_to_u64(bx),x); cvm_u64_to_h(cvm_h_to_u64(y)+cvm_h_to_u64(by),y); cvm_zero(o); memcpy(o,x,8); memcpy(o+8,y,8); cvm_push(o); cnext(); }
