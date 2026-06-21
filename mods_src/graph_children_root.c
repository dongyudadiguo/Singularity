#include "net_ops.h"
static H Z={0};
__declspec(dllexport) void run(void) { H o; u8*d=0; u32 len=0; cvm_zero(o); if(net_children(Z,&d,&len)){block_write(d,len,o);free(d);} cvm_push(o); cnext(); }
