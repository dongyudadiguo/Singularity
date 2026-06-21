#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s&&s->sp>=2&&s->sp<CVM_STACK_CAP){memcpy(s->stack[s->sp],s->stack[s->sp-1],32); memcpy(s->stack[s->sp-1],s->stack[s->sp-2],32); memcpy(s->stack[s->sp-2],s->stack[s->sp],32); s->sp++;} cnext(); }
