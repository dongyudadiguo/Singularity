#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,rec_h,o; cvm_pop(rec_h); cvm_pop(h); u32 len=0,rlen=0; u8*d=block_read(h,&len); u8*r=block_read(rec_h,&rlen); cvm_zero(o); if(d&&r){ u8*b=malloc(len+rlen+1); if(b){memcpy(b,d,len); memcpy(b+len,r,rlen); block_write(b,len+rlen,o); free(b);} } if(d)free(d); if(r)free(r); cvm_push(o); cnext(); }
