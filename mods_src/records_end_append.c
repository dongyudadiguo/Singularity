#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); u8 z[32]={0}; cvm_zero(o); if(d){u8*b=malloc(len+32+1); if(b){memcpy(b,d,len); memcpy(b+len,z,32); block_write(b,len+32,o); free(b);} free(d);} cvm_push(o); cnext(); }
