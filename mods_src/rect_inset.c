#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,d,o,x,y,w,h; cvm_pop(d); cvm_pop(r); cvm_zero(x);cvm_zero(y);cvm_zero(w);cvm_zero(h);memcpy(x,r,8);memcpy(y,r+8,8);memcpy(w,r+16,8);memcpy(h,r+24,8);u64 dv=cvm_h_to_u64(d);cvm_u64_to_h(cvm_h_to_u64(x)+dv,x);cvm_u64_to_h(cvm_h_to_u64(y)+dv,y);u64 wv=cvm_h_to_u64(w),hv=cvm_h_to_u64(h);cvm_u64_to_h(wv>dv*2?wv-dv*2:0,w);cvm_u64_to_h(hv>dv*2?hv-dv*2:0,h);cvm_zero(o);memcpy(o,x,8);memcpy(o+8,y,8);memcpy(o+16,w,8);memcpy(o+24,h,8);cvm_push(o);cnext(); }
