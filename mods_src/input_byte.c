#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H o; cvm_zero(o); int c=getchar(); if(c!=EOF)o[0]=(u8)c; cvm_push(o); cnext(); }
