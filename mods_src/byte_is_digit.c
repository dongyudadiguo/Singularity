#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); u64 v=cvm_h_to_u64(h); cvm_zero(o); if(v>='0'&&v<='9')o[0]=1; cvm_push(o); cnext(); }
