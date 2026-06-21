#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,r,o,a,b; cvm_pop(r); cvm_pop(h); cvm_zero(a); cvm_zero(b); memcpy(a,r,8); memcpy(b,r+8,8); u64 off=cvm_h_to_u64(a),n=cvm_h_to_u64(b); u32 len=0; u8*d=block_read(h,&len); cvm_zero(o); if(d){ if(off<len){ if(off+n>len)n=len-off; block_write(d+off,(u32)n,o);} free(d);} cvm_push(o); cnext(); }
