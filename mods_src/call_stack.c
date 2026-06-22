#include "../cvm_state.h"
#include "../block.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H target;
    u32 len;
    cvm_pop(target);
    CvmCallFrame frame;
    cframe_save(s, &frame);
    if (s) memcpy(s->cur_hash, target, 32);
    u8 *p = block(&target, &len);
    if (!p) { cframe_restore(s, &frame); cnext(); return; }
    if (s) {
        jmp_buf jb;
        s->ret_jb = &jb;
        if (setjmp(jb) == 0) cbegin(p, len);
        cframe_restore(s, &frame);
    }
    block(0, 0);
    cvm_push(target);
    cnext();
}
