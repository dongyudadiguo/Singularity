#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H tok,pay,o; cvm_pop(pay); cvm_pop(tok); u8 b[68]; memcpy(b,tok,32); b[32]=36; b[33]=0; b[34]=0; b[35]=0; memcpy(b+36,pay,32); cvm_zero(o); block_write(b,68,o); cvm_push(o); cnext(); }
