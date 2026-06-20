#include "io_parse.h"
__declspec(dllexport) void run(void) { H h,out; u32 len=0; CvmState*s=cvm_state(); cvm_pop(h); u8*d=block_read(h,&len); cvm_zero(out); if(s&&d&&len==s->payload_len&&memcmp(d,s->payload,len)==0) out[0]=1; if(d)free(d); cvm_push(out); cnext(); }
