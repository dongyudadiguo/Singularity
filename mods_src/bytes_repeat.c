#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H byte_h,n_h,o; cvm_pop(n_h); cvm_pop(byte_h); u64 n=cvm_h_to_u64(n_h); cvm_zero(o); u8*b=malloc((u32)n+1); if(b){memset(b,(u8)cvm_h_to_u64(byte_h),(u32)n); block_write(b,(u32)n,o); free(b);} cvm_push(o); cnext(); }
