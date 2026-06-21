#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; cvm_zero(o); if(s&&s->payload)block_write(s->payload,s->payload_len,o); cvm_push(o); cnext(); }
