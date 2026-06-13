#include "cvm_mod.h"

static Host *G;

static int hexv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

static int st_push(u8 *d, uint32_t n) { G->push(d, n); return 0; }

static int st_pop(u8 *d, uint32_t n) {
    Buf a = G->pop();
    free(a.p);
    return 0;
}

static int st_dup(u8 *d, uint32_t n) {
    Buf *a = G->top();
    G->push(a->p, a->n);
    return 0;
}

static int st_swap(u8 *d, uint32_t n) {
    Buf a = G->pop(), b = G->pop();
    G->push(a.p, a.n);
    G->push(b.p, b.n);
    free(a.p); free(b.p);
    return 0;
}

static int by_len(u8 *d, uint32_t n) {
    Buf a = G->pop();
    u8 o[4];
    wr32(o, a.n);
    G->push(o, 4);
    free(a.p);
    return 0;
}

static int by_cat(u8 *d, uint32_t n) {
    Buf b = G->pop(), a = G->pop(), o = mbuf(a.n + b.n);
    memcpy(o.p, a.p, a.n);
    memcpy(o.p + a.n, b.p, b.n);
    G->push(o.p, o.n);
    free(a.p); free(b.p); free(o.p);
    return 0;
}

static int by_slice(u8 *d, uint32_t n) {
    Buf ln = G->pop(), off = G->pop(), a = G->pop();
    G->push(a.p + rd32(off.p), rd32(ln.p));
    free(a.p); free(off.p); free(ln.p);
    return 0;
}

static int by_cmp(u8 *d, uint32_t n) {
    Buf b = G->pop(), a = G->pop();
    u8 o[4];
    wr32(o, a.n == b.n && !memcmp(a.p, b.p, a.n));
    G->push(o, 4);
    free(a.p); free(b.p);
    return 0;
}

static int by_take32(u8 *d, uint32_t n) {
    Buf a = G->pop();
    G->push(a.p, 32);
    free(a.p);
    return 0;
}

static int by_hex(u8 *d, uint32_t n) {
    static char *h = "0123456789abcdef";
    Buf a = G->pop(), o = mbuf(a.n * 2);
    for (DWORD i = 0; i < a.n; i++) {
        o.p[i * 2] = h[a.p[i] >> 4];
        o.p[i * 2 + 1] = h[a.p[i] & 15];
    }
    G->push(o.p, o.n);
    free(a.p); free(o.p);
    return 0;
}

static int by_unhex(u8 *d, uint32_t n) {
    Buf a = G->pop(), o = mbuf(a.n / 2);
    for (DWORD i = 0; i < o.n; i++)
        o.p[i] = hexv(a.p[i * 2]) * 16 + hexv(a.p[i * 2 + 1]);
    G->push(o.p, o.n);
    free(a.p); free(o.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:ST:PUSH", st_push);
    h->op_name("CVM1:ST:POP", st_pop);
    h->op_name("CVM1:ST:DUP", st_dup);
    h->op_name("CVM1:ST:SWAP", st_swap);

    h->op_name("CVM1:BY:LEN", by_len);
    h->op_name("CVM1:BY:CAT", by_cat);
    h->op_name("CVM1:BY:SLICE", by_slice);
    h->op_name("CVM1:BY:CMP", by_cmp);
    h->op_name("CVM1:BY:TAKE32", by_take32);
    h->op_name("CVM1:BY:HEX", by_hex);
    h->op_name("CVM1:BY:UNHEX", by_unhex);
}
