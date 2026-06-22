#include "../cvm_state.h"
#include "../block.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (!s || !s->payload || s->payload_len < 32) { cnext(); return; }
    H cond;
    cvm_pop(cond);
    if (!cvm_truth(cond)) { cnext(); return; }
    H target;
    u32 len;
    memcpy(target, s->payload, 32);
    CvmCallFrame frame;
    cframe_save(s, &frame);
    if (s) memcpy(s->cur_hash, target, 32);
    u8 *p = block_read(target, &len);
    if (!p) { cframe_restore(s, &frame); cnext(); return; }
    if (s) {
        jmp_buf jb;
        s->ret_jb = &jb;
        if (setjmp(jb) == 0) cbegin(p, len);
        cframe_restore(s, &frame);
    }
    free(p);
    cnext();
}
