#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H out;
    cvm_zero(out);
    if (s && s->payload) block_write(s->payload, s->payload_len, out);
    cvm_push(out);
    cnext();
}
