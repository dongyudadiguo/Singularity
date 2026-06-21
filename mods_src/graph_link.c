#include "net_ops.h"
__declspec(dllexport) void run(void) { H p,c,o; cvm_pop(c); cvm_pop(p); cvm_zero(o); if(net_edge(p,c))o[0]=1; cvm_push(o); cnext(); }
