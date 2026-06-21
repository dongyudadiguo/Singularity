#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,o; cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h); cvm_zero(o); if(d&&off+4<=len){ u64 v=(u64)d[off]|((u64)d[off+1]<<8)|((u64)d[off+2]<<16)|((u64)d[off+3]<<24); cvm_u64_to_h(v,o);} if(d)free(d); cvm_push(o); cnext(); }
