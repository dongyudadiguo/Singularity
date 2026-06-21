#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); cvm_zero(o); block_write(h,32,o); cvm_push(o); cnext(); }
