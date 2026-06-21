#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u32 len=0; u8*d=block_read(h,&len); if(d)free(d); cvm_u64_to_h(len,o); cvm_push(o); cnext(); }
