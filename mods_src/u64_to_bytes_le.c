#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H a,o; cvm_pop(a); u64 v=cvm_h_to_u64(a); u8 b[8]; for(int i=0;i<8;i++) b[i]=(u8)(v>>(i*8)); cvm_zero(o); block_write(b,8,o); cvm_push(o); cnext(); }
