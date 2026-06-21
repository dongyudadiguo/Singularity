#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); cvm_zero(o); if(d){block_write(d,len,o); free(d);} cvm_push(o); cnext(); }
