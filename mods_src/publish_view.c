#include "net_ops.h"
static H ROOT_ZERO = {0};
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s) net_edge(ROOT_ZERO,s->view_hash); cnext(); }
