#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); u8 buf[64]; memcpy(buf,a,32); memcpy(buf+32,b,32); cvm_zero(o); block_write(buf,64,o); cvm_push(o); cnext(); }
