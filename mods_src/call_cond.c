#include "../cvm_state.h"
#include "../block.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H cond, target;
    u32 len;
    cvm_pop(cond);
    cvm_pop(target);
    if (!cvm_truth(cond)) { cnext(); return; }
    CvmState *s = cvm_state();
    if (s) memcpy(s->cur_hash, target, 32);
    u8 *p = block(&target, &len);
    if (!p) { cnext(); return; }
    if (s) {
        jmp_buf jb;
        jmp_buf *old = s->ret_jb;
        s->ret_jb = &jb;
        if (setjmp(jb) == 0) cbegin(p, len);
        s->ret_jb = old;
    }
    block(0, 0);
    if (s) memcpy(s->cur_hash, target, 32);
    cvm_push(target);
    cnext();
}
