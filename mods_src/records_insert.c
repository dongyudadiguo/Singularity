#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,idx_h,rec_h,o; cvm_pop(rec_h); cvm_pop(idx_h); cvm_pop(h); u32 len=0,rlen=0,off=0,idx=0,target=(u32)cvm_h_to_u64(idx_h); u8*d=block_read(h,&len); u8*r=block_read(rec_h,&rlen); cvm_zero(o); if(d&&r){ while(off+36<=len&&idx<target){ int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)break; u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp<4||off+32+sp>len)break; off+=32+sp; idx++; } u8*b=malloc(len+rlen+1); if(b){memcpy(b,d,off); memcpy(b+off,r,rlen); memcpy(b+off+rlen,d+off,len-off); block_write(b,len+rlen,o); free(b);} } if(d)free(d); if(r)free(r); cvm_push(o); cnext(); }
