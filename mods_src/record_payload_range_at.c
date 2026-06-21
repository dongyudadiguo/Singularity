#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,o,s,l; cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h); cvm_zero(s); cvm_zero(l); if(d&&off+36<=len){u32 sp=(u32)d[off+32]|((u32)d[off+33]<<8)|((u32)d[off+34]<<16)|((u32)d[off+35]<<24); if(sp>=4&&off+32+sp<=len){cvm_u64_to_h(off+36,s); cvm_u64_to_h(sp-4,l);}} if(d)free(d); cvm_zero(o); memcpy(o,s,8); memcpy(o+8,l,8); cvm_push(o); cnext(); }
