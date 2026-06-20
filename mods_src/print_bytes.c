#include "io_parse.h"
__declspec(dllexport) void run(void) { H h; u32 len=0; cvm_pop(h); u8 *d=block_read(h,&len); if(d){ fwrite(d,1,len,stdout); free(d);} cnext(); }
