#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H sp,o; cvm_pop(sp); cvm_u64_to_h(cvm_h_to_u64(sp)+32,o); cvm_push(o); cnext(); }
