#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H tok,o; cvm_pop(tok); u8 b[36]; memcpy(b,tok,32); b[32]=4; b[33]=0; b[34]=0; b[35]=0; cvm_zero(o); block_write(b,36,o); cvm_push(o); cnext(); }
