#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,idx_h,o; cvm_pop(idx_h); cvm_pop(h); u32 len=0,off=0,idx=0,target=(u32)cvm_h_to_u64(idx_h); u8*d=block_read(h,&len); u64 out=0; if(d){ while(off+32<=len){ if(idx==target){out=off;break;} int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)break; if(off+36>len)break; u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp<4||off+32+sp>len)break; off+=32+sp; idx++; } free(d); } cvm_u64_to_h(out,o); cvm_push(o); cnext(); }
