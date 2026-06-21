#include "net_ops.h"
static H Z={0};
__declspec(dllexport) void run(void) { H c,o; cvm_pop(c); cvm_zero(o); if(net_edge(Z,c))o[0]=1; cvm_push(o); cnext(); }
