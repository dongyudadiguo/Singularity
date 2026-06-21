#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,n_h,val_h,o; cvm_pop(val_h); cvm_pop(n_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 n=cvm_h_to_u64(n_h); cvm_zero(o); if(d){ if(n<len)n=len; u8*buf=malloc((u32)n+1); if(buf){memcpy(buf,d,len); memset(buf+len,(u8)cvm_h_to_u64(val_h),(u32)n-len); block_write(buf,(u32)n,o); free(buf);} free(d);} cvm_push(o); cnext(); }
