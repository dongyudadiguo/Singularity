#include "block_chain.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H target,out; cvm_pop(target); cvm_zero(out); if(s&&s->payload_len>=32){u8 buf[68]; memcpy(buf,s->payload,32); buf[32]=36;buf[33]=0;buf[34]=0;buf[35]=0; memcpy(buf+36,target,32); block_write(buf,68,out);} cvm_push(out); cnext(); }
