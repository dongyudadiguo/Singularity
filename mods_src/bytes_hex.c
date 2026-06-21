#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); static char hx[]="0123456789abcdef"; cvm_zero(o); if(d){u8*b=malloc(len*2+1); if(b){for(u32 i=0;i<len;i++){b[i*2]=hx[d[i]>>4];b[i*2+1]=hx[d[i]&15];}block_write(b,len*2,o);free(b);} free(d);} cvm_push(o); cnext(); }
