#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s&&s->view_sp<CVM_VIEW_CAP){memcpy(s->view_hash_stack[s->view_sp],s->view_hash,32);s->view_index_stack[s->view_sp]=s->view_index;s->view_sp++;} cnext(); }
