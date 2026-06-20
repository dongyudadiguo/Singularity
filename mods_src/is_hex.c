#include "io_parse.h"
__declspec(dllexport) void run(void) { H h,tmp,out; u32 len=0; cvm_pop(h); u8*d=block_read(h,&len); cvm_zero(out); if(d&&parse_hex_hash_bytes(d,len,tmp)) out[0]=1; if(d)free(d); cvm_push(out); cnext(); }
