#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H o; cvm_zero(o); block_write((u8*)"",0,o); cvm_push(o); cnext(); }
