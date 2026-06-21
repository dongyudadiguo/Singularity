#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; cvm_zero(o); if(s&&s->payload_len) o[0]=s->payload[0]; cvm_push(o); cnext(); }
