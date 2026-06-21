#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,n_h,o; cvm_pop(n_h); cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h),n=cvm_h_to_u64(n_h); cvm_zero(o); if(d){ if(off>len)off=len; if(off+n>len)n=len-off; u8*b=malloc(len-(u32)n+1); if(b){memcpy(b,d,(u32)off); memcpy(b+off,d+off+n,len-(u32)off-(u32)n); block_write(b,len-(u32)n,o); free(b);} free(d);} cvm_push(o); cnext(); }
