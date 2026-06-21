#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,x_h,y_h,o,t; cvm_pop(y_h); cvm_pop(x_h); cvm_pop(r); cvm_zero(t); memcpy(t,r,8); u64 x0=cvm_h_to_u64(t); cvm_zero(t); memcpy(t,r+8,8); u64 y0=cvm_h_to_u64(t); cvm_zero(t); memcpy(t,r+16,8); u64 w=cvm_h_to_u64(t); cvm_zero(t); memcpy(t,r+24,8); u64 h=cvm_h_to_u64(t); u64 x=cvm_h_to_u64(x_h),y=cvm_h_to_u64(y_h); cvm_u64_to_h(x>=x0&&y>=y0&&x<x0+w&&y<y0+h,o); cvm_push(o); cnext(); }
