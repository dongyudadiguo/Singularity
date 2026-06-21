#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H h; cvm_pop(h); putchar((int)(u8)cvm_h_to_u64(h)); fflush(stdout); cnext(); }
