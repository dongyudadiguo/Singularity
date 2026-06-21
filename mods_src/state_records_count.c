#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; u32 len=0,off=0,n=0; u8*d=s?block_read(s->view_hash,&len):0; if(d){while(off+36<=len){int z=1;for(int i=0;i<32;i++)if(d[off+i]){z=0;break;}if(z)break;u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24);if(sp<4||off+32+sp>len)break;off+=32+sp;n++;}free(d);} cvm_u64_to_h(n,o); cvm_push(o); cnext(); }
