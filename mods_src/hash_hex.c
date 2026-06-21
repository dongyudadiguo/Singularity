#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { H h,o; cvm_pop(h); static char hx[]="0123456789abcdef"; u8 b[64]; for(int i=0;i<32;i++){b[i*2]=hx[h[i]>>4];b[i*2+1]=hx[h[i]&15];} cvm_zero(o); block_write(b,64,o); cvm_push(o); cnext(); }
