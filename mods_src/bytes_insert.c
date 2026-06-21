#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,part_h,o; cvm_pop(part_h); cvm_pop(off_h); cvm_pop(h); u32 len=0,plen=0; u8*d=block_read(h,&len); u8*p=block_read(part_h,&plen); u64 off=cvm_h_to_u64(off_h); cvm_zero(o); if(d&&p){ if(off>len)off=len; u8*b=malloc(len+plen+1); if(b){memcpy(b,d,(u32)off); memcpy(b+off,p,plen); memcpy(b+off+plen,d+off,len-(u32)off); block_write(b,len+plen,o); free(b);} } if(d)free(d); if(p)free(p); cvm_push(o); cnext(); }
