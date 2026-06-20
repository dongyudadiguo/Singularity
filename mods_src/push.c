#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H v;
    cvm_zero(v);
    if (s && s->payload && s->payload_len) {
        u32 n = s->payload_len > 32 ? 32 : s->payload_len;
        memcpy(v, s->payload, n);
    }
    cvm_push(v);
    cnext();
}
