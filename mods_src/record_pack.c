#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H tok,pay_h,o; cvm_pop(pay_h); cvm_pop(tok); u32 len=0; u8*p=block_read(pay_h,&len); u32 sp=len+4,total=36+len; u8*b=malloc(total+1); cvm_zero(o); if(b){memcpy(b,tok,32); b[32]=(u8)sp; b[33]=(u8)(sp>>8); b[34]=(u8)(sp>>16); b[35]=(u8)(sp>>24); if(p)memcpy(b+36,p,len); block_write(b,total,o); free(b);} if(p)free(p); cvm_push(o); cnext(); }
