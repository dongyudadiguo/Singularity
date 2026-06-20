#include "io_parse.h"
__declspec(dllexport) void run(void) { H out; cvm_zero(out); int c=getchar(); if(c!=EOF) out[0]=(u8)c; cvm_push(out); cnext(); }
