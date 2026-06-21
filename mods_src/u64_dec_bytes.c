#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); char b[32]; int n=sprintf(b,"%llu",cvm_h_to_u64(h)); cvm_zero(o); block_write((u8*)b,(u32)n,o); cvm_push(o); cnext(); }
