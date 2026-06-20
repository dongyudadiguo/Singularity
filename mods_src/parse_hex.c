#include "io_parse.h"
__declspec(dllexport) void run(void) { H h,out; u32 len=0; cvm_pop(h); u8*d=block_read(h,&len); if(!d||!parse_hex_hash_bytes(d,len,out)) cvm_zero(out); if(d)free(d); cvm_push(out); cnext(); }
