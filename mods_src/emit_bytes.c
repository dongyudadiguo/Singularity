#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); if(d){fwrite(d,1,len,stdout); fflush(stdout); free(d);} cnext(); }
