#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s&&s->sp>=3){H t; memcpy(t,s->stack[s->sp-3],32); memcpy(s->stack[s->sp-3],s->stack[s->sp-2],32); memcpy(s->stack[s->sp-2],s->stack[s->sp-1],32); memcpy(s->stack[s->sp-1],t,32);} cnext(); }
