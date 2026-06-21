#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u64 n=0; if(d&&len>=4)n=(u64)d[0]<<24|(u64)d[1]<<16|(u64)d[2]<<8|d[3]; if(d)free(d); cvm_u64_to_h(n,o); cvm_push(o); cnext(); }
