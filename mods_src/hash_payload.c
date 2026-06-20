#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H out;
    cvm_zero(out);
    if (s && s->payload) cvm_sha256(s->payload, s->payload_len, out);
    cvm_push(out);
}
