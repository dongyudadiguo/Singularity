#include "io_parse.h"
__declspec(dllexport) void run(void) { CvmState *s=cvm_state(); if(s&&s->payload) fwrite(s->payload,1,s->payload_len,stdout); cnext(); }
