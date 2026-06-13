#include "cvm_mod.h"

static Host *G;

static DWORD off(u8 *p, DWORD idx) {
    DWORD o = 4;
    while (idx--) o += 4 + rd32(p + o);
    return o;
}

static void rec(u8 *p, Buf x) {
    wr32(p, x.n);
    memcpy(p + 4, x.p, x.n);
}

static int lst_new(u8 *d, uint32_t n) {
    u8 z[4] = {0};
    G->push(z, 4);
    return 0;
}

static int lst_count(u8 *d, uint32_t n) {
    Buf l;
    u8 o[4];

    if (n) l.p = d, l.n = n;
    else l = G->pop();

    wr32(o, rd32(l.p));
    G->push(o, 4);

    if (!n) free(l.p);
    return 0;
}

static int lst_get(u8 *d, uint32_t n) {
    Buf idx = G->pop();
    Buf l;

    if (n) l.p = d, l.n = n;
    else l = G->pop();

    DWORD o = off(l.p, rd32(idx.p));
    G->push(l.p + o + 4, rd32(l.p + o));

    free(idx.p);
    if (!n) free(l.p);
    return 0;
}

static int lst_push(u8 *d, uint32_t n) {
    Buf x = G->pop(), l = G->pop();
    Buf o = mbuf(l.n + 4 + x.n);

    memcpy(o.p, l.p, l.n);
    wr32(o.p, rd32(o.p) + 1);
    rec(o.p + l.n, x);

    G->push(o.p, o.n);

    free(x.p);
    free(l.p);
    free(o.p);
    return 0;
}

static int lst_ins(u8 *d, uint32_t n) {
    Buf x = G->pop(), idx = G->pop(), l = G->pop();
    DWORD p = off(l.p, rd32(idx.p));
    Buf o = mbuf(l.n + 4 + x.n);

    memcpy(o.p, l.p, p);
    rec(o.p + p, x);
    memcpy(o.p + p + 4 + x.n, l.p + p, l.n - p);
    wr32(o.p, rd32(o.p) + 1);

    G->push(o.p, o.n);

    free(x.p);
    free(idx.p);
    free(l.p);
    free(o.p);
    return 0;
}

static int lst_set(u8 *d, uint32_t n) {
    Buf x = G->pop(), idx = G->pop(), l = G->pop();
    DWORD p = off(l.p, rd32(idx.p));
    DWORD old = 4 + rd32(l.p + p);
    Buf o = mbuf(l.n - old + 4 + x.n);

    memcpy(o.p, l.p, p);
    rec(o.p + p, x);
    memcpy(o.p + p + 4 + x.n, l.p + p + old, l.n - p - old);

    G->push(o.p, o.n);

    free(x.p);
    free(idx.p);
    free(l.p);
    free(o.p);
    return 0;
}

static int lst_del(u8 *d, uint32_t n) {
    Buf idx = G->pop(), l = G->pop();
    DWORD p = off(l.p, rd32(idx.p));
    DWORD old = 4 + rd32(l.p + p);
    Buf o = mbuf(l.n - old);

    memcpy(o.p, l.p, p);
    memcpy(o.p + p, l.p + p + old, l.n - p - old);
    wr32(o.p, rd32(l.p) - 1);

    G->push(o.p, o.n);

    free(idx.p);
    free(l.p);
    free(o.p);
    return 0;
}

static int lst_join(u8 *d, uint32_t n) {
    Buf sep = G->pop(), l = G->pop();
    DWORD c = rd32(l.p), total = 0, p = 4;

    for (DWORD i = 0; i < c; i++) {
        total += rd32(l.p + p);
        p += 4 + rd32(l.p + p);
    }

    if (c) total += sep.n * (c - 1);

    Buf o = mbuf(total);
    DWORD q = 0;
    p = 4;

    for (DWORD i = 0; i < c; i++) {
        DWORD m = rd32(l.p + p);
        if (i) {
            memcpy(o.p + q, sep.p, sep.n);
            q += sep.n;
        }
        memcpy(o.p + q, l.p + p + 4, m);
        q += m;
        p += 4 + m;
    }

    G->push(o.p, o.n);

    free(sep.p);
    free(l.p);
    free(o.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:LST:NEW", lst_new);
    h->op_name("CVM1:LST:COUNT", lst_count);
    h->op_name("CVM1:LST:GET", lst_get);
    h->op_name("CVM1:LST:PUSH", lst_push);
    h->op_name("CVM1:LST:INS", lst_ins);
    h->op_name("CVM1:LST:SET", lst_set);
    h->op_name("CVM1:LST:DEL", lst_del);
    h->op_name("CVM1:LST:JOIN", lst_join);
}
