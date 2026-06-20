#include "../cvm_state.h"
#include "../block.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H target;
    u32 len;
    CvmState *s = cvm_state();
    cvm_pop(target);
    if (s) memcpy(s->cur_hash, target, 32);
    u8 *p = block(&target, &len);
    if (!p) return;
    while (cont(&p)) {
        if (s && s->ret) break;
    }
    block(0, 0);
    if (s) memcpy(s->cur_hash, target, 32);
    cvm_push(target);
}
