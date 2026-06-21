#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s&&s->view_index)s->view_index--; cnext(); }
