#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H tok,val,o; cvm_pop(val); cvm_pop(tok); u8 b[44]; memcpy(b,tok,32); b[32]=12; b[33]=0; b[34]=0; b[35]=0; u64 v=cvm_h_to_u64(val); for(int i=0;i<8;i++)b[36+i]=(u8)(v>>(i*8)); cvm_zero(o); block_write(b,44,o); cvm_push(o); cnext(); }
