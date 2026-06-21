#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H v_h,o; cvm_pop(v_h); u64 v=cvm_h_to_u64(v_h); u8 b[4]={(u8)v,(u8)(v>>8),(u8)(v>>16),(u8)(v>>24)}; cvm_zero(o); block_write(b,4,o); cvm_push(o); cnext(); }
