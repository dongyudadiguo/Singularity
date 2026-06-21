#include "net_ops.h"
__declspec(dllexport) void run(void) { H p,o; u8*d=0; u32 len=0; cvm_pop(p); cvm_zero(o); if(net_children(p,&d,&len)){block_write(d,len,o);free(d);} cvm_push(o); cnext(); }
