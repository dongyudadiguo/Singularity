#include "net_ops.h"

static H BOOT_KEY_BR = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};

__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H target;
    u32 len = 0;
    cvm_zero(target);
    if (!net_uget(BOOT_KEY_BR, target)) return;
    if (s) {
        memcpy(s->view_hash, target, 32);
        memcpy(s->cur_hash, target, 32);
    }
    u8 *p = block(&target, &len);
    if (!p) return;
    if (s) {
        jmp_buf jb;
        jmp_buf *old = s->ret_jb;
        s->ret_jb = &jb;
        if (setjmp(jb) == 0) cbegin(p, len);
        s->ret_jb = old;
    }
    block(0, 0);
}
