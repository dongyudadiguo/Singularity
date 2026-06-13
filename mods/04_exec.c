#include "cvm_mod.h"

static Host *G;

static void arg32(u8 *d, uint32_t n, u8 out[32]) {
    if (n >= 32) {
        memcpy(out, d, 32);
    } else {
        Buf a = G->pop();
        memcpy(out, a.p, 32);
        free(a.p);
    }
}

static int ex_run(u8 *d, uint32_t n) {
    u8 h[32];
    arg32(d, n, h);
    G->run(h);
    return 1;
}

static int ex_enter(u8 *d, uint32_t n) {
    u8 h[32];
    arg32(d, n, h);
    G->enter(h);
    return 1;
}

static int ex_adv(u8 *d, uint32_t n) {
    G->adv();
    return 1;
}

static int ov_set(u8 *d, uint32_t n) {
    if (n >= 32) {
        G->override(d, d + 32, n - 32);
    } else {
        Buf file = G->pop(), key = G->pop();
        G->override(key.p, file.p, file.n);
        free(file.p); free(key.p);
    }
    return 0;
}

static int ov_touch(u8 *d, uint32_t n) {
    G->touch();
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:RUN", ex_run);
    h->op_name("CVM1:ENTER", ex_enter);
    h->op_name("CVM1:ADV", ex_adv);

    h->op_name("CVM1:OV:SET", ov_set);
    h->op_name("CVM1:OV:TOUCH", ov_touch);
}
