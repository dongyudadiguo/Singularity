#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,o; cvm_pop(r); u64 a=0,b=0; for(int i=0;i<8;i++){a|=((u64)r[i])<<(i*8); b|=((u64)r[i+8])<<(i*8);} cvm_u64_to_h(a+b,o); cvm_push(o); cnext(); }
