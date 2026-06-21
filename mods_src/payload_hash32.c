#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; cvm_zero(o); if(s&&s->payload){u32 n=s->payload_len<32?s->payload_len:32; memcpy(o,s->payload,n);} cvm_push(o); cnext(); }
