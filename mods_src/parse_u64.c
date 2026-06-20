#include "io_parse.h"
__declspec(dllexport) void run(void) { H h,out; u32 len=0; u64 v=0; cvm_pop(h); u8*d=block_read(h,&len); if(d&&parse_u64_bytes(d,len,&v)) cvm_u64_to_h(v,out); else cvm_zero(out); if(d)free(d); cvm_push(out); cnext(); }
