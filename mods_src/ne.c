#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); cvm_u64_to_h(memcmp(a,b,32)!=0,o); cvm_push(o); cnext(); }
