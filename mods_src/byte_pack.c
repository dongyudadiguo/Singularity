#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H a,o; cvm_pop(a); u8 b=(u8)cvm_h_to_u64(a); cvm_zero(o); block_write(&b,1,o); cvm_push(o); cnext(); }
