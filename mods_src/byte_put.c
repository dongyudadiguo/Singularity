#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,off_h,val_h,o; cvm_pop(val_h); cvm_pop(off_h); cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 off=cvm_h_to_u64(off_h); cvm_zero(o); if(d&&off<len){ d[off]=(u8)cvm_h_to_u64(val_h); block_write(d,len,o); } if(d)free(d); cvm_push(o); cnext(); }
