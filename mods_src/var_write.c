#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H id, val;
    if (!s) return;
    cvm_sha256(s->payload, s->payload_len, id);
    cvm_pop(val);
    cvm_var_write(id, val);
    cnext();
}
