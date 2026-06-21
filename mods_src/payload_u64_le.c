#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); H o; u64 v=0; if(s&&s->payload){u32 n=s->payload_len<8?s->payload_len:8; for(u32 i=0;i<n;i++) v|=((u64)s->payload[i])<<(i*8);} cvm_u64_to_h(v,o); cvm_push(o); cnext(); }
