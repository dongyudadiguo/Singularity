#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,v_h,o; cvm_pop(v_h); cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h),v=cvm_h_to_u64(v_h); cvm_zero(o); if(d&&off+4<=len){ d[off]=(u8)v; d[off+1]=(u8)(v>>8); d[off+2]=(u8)(v>>16); d[off+3]=(u8)(v>>24); block_write(d,len,o);} if(d)free(d); cvm_push(o); cnext(); }
