#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,o; cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h); cvm_zero(o); if(!d||off+32>len)o[0]=1; else { int z=1; for(int i=0;i<32;i++) if(d[off+i]){z=0;break;} if(z)o[0]=1; } if(d)free(d); cvm_push(o); cnext(); }
