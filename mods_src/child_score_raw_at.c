#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,idx_h,o; cvm_pop(idx_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 idx=cvm_h_to_u64(idx_h),off=4+idx*40+32,v=0; if(d&&off+8<=len){for(int i=0;i<8;i++)v=(v<<8)|d[off+i];} if(d)free(d); cvm_u64_to_h(v,o); cvm_push(o); cnext(); }
