#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,o; cvm_pop(a); u64 v=cvm_h_to_u64(a); cvm_u64_to_h(v?v-1:0, o); cvm_push(o); cnext(); }
