#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H h,i; cvm_pop(i); cvm_pop(h); if(s){memcpy(s->view_hash,h,32); s->view_index=cvm_h_to_u64(i);} cnext(); }
