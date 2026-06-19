#include "../cvm_state.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); cvm_u64_to_h(cvm_truth(a)&&cvm_truth(b), o); cvm_push(o); }
