#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0,off=0,n=0; u8*d=block_read(h,&len); if(d){ while(off+32<=len){ int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)break; if(off+36>len)break; u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp<4||off+32+sp>len)break; off+=32+sp; n++; } free(d); } cvm_u64_to_h(n,o); cvm_push(o); cnext(); }
