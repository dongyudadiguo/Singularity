#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,idx_h,o,start,len_h; cvm_pop(idx_h); cvm_pop(h); u32 blen=0,off=0,idx=0,target=(u32)cvm_h_to_u64(idx_h); u8*d=block_read(h,&blen); cvm_zero(start); cvm_zero(len_h); if(d){ while(off+36<=blen){ int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)break; u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp<4||off+32+sp>blen)break; if(idx==target){cvm_u64_to_h(off,start); cvm_u64_to_h(32+sp,len_h); break;} off+=32+sp; idx++; } free(d); } cvm_zero(o); memcpy(o,start,8); memcpy(o+8,len_h,8); cvm_push(o); cnext(); }
