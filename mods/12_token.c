#include "cvm_mod.h"

static Host *G;

static void tok(u8 *s, DWORD n, u8 o[32]) {
    memset(o, 0, 32);
    memcpy(o, s, n > 32 ? 32 : n);
}

static int isz(u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

static int tok_zero(u8 *d, uint32_t n) {
    u8 z[32] = {0};
    G->push(z, 32);
    return 0;
}

static int tok_make(u8 *d, uint32_t n) {
    u8 o[32];

    if (n) tok(d, n, o);
    else {
        Buf a = G->pop();
        tok(a.p, a.n, o);
        free(a.p);
    }

    G->push(o, 32);
    return 0;
}

static int tok_text(u8 *d, uint32_t n) {
    Buf a;
    DWORD l = 0;

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    while (l < a.n && l < 32 && a.p[l]) l++;
    G->push(a.p, l);

    if (!n) free(a.p);
    return 0;
}

static int tok_iszero(u8 *d, uint32_t n) {
    Buf a;
    u8 o[4];

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    wr32(o, isz(a.p));
    G->push(o, 4);

    if (!n) free(a.p);
    return 0;
}

static int tok_eq(u8 *d, uint32_t n) {
    Buf b = G->pop(), a = G->pop();
    u8 o[4];

    wr32(o, a.n >= 32 && b.n >= 32 && !memcmp(a.p, b.p, 32));
    G->push(o, 4);

    free(a.p);
    free(b.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:TOK:ZERO", tok_zero);
    h->op_name("CVM1:TOK:MAKE", tok_make);
    h->op_name("CVM1:TOK:TEXT", tok_text);
    h->op_name("CVM1:TOK:ISZERO", tok_iszero);
    h->op_name("CVM1:TOK:EQ", tok_eq);
}
