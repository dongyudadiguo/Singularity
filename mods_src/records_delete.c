#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,idx_h,o; cvm_pop(idx_h); cvm_pop(h); u32 len=0,off=0,idx=0,target=(u32)cvm_h_to_u64(idx_h),end=0; u8*d=block_read(h,&len); cvm_zero(o); if(d){ while(off+36<=len){ int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)break; u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp<4||off+32+sp>len)break; end=off+32+sp; if(idx==target)break; off=end; idx++; } if(idx==target&&end>off){ u8*b=malloc(len-(end-off)+1); if(b){memcpy(b,d,off); memcpy(b+off,d+end,len-end); block_write(b,len-(end-off),o); free(b);} } free(d); } cvm_push(o); cnext(); }
