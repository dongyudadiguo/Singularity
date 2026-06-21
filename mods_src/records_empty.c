#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H o; u8 z[32]={0}; cvm_zero(o); block_write(z,32,o); cvm_push(o); cnext(); }
