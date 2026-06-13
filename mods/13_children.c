#include "cvm_mod.h"

static Host *G;

static uint32_t be32(u8 *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint64_t be64(u8 *p) {
    uint64_t x = 0;
    for (int i = 0; i < 8; i++) x = (x << 8) | p[i];
    return x;
}

static Buf arg(u8 *d, uint32_t n, int *own) {
    Buf a;
    if (n) {
        a.p = d;
        a.n = n;
        *own = 0;
    } else {
        a = G->pop();
        *own = 1;
    }
    return a;
}

static DWORD row(DWORD i) {
    return 4 + i * 40;
}

static int ch_count(u8 *d, uint32_t n) {
    int own;
    Buf r = arg(d, n, &own);
    u8 o[4];

    wr32(o, be32(r.p));
    G->push(o, 4);

    if (own) free(r.p);
    return 0;
}

static int ch_first(u8 *d, uint32_t n) {
    int own;
    Buf r = arg(d, n, &own);

    G->push(r.p + 4, 32);

    if (own) free(r.p);
    return 0;
}

static int ch_hash(u8 *d, uint32_t n) {
    Buf idx = G->pop();
    int own;
    Buf r = arg(d, n, &own);

    G->push(r.p + row(rd32(idx.p)), 32);

    free(idx.p);
    if (own) free(r.p);
    return 0;
}

static int ch_score(u8 *d, uint32_t n) {
    Buf idx = G->pop();
    int own;
    Buf r = arg(d, n, &own);
    u8 o[8];

    wr64(o, be64(r.p + row(rd32(idx.p)) + 32));
    G->push(o, 8);

    free(idx.p);
    if (own) free(r.p);
    return 0;
}

static int ch_row(u8 *d, uint32_t n) {
    Buf idx = G->pop();
    int own;
    Buf r = arg(d, n, &own);
    u8 o[40];
    DWORD p = row(rd32(idx.p));

    memcpy(o, r.p + p, 32);
    wr64(o + 32, be64(r.p + p + 32));
    G->push(o, 40);

    free(idx.p);
    if (own) free(r.p);
    return 0;
}

static int ch_hashes(u8 *d, uint32_t n) {
    int own;
    Buf r = arg(d, n, &own);
    DWORD c = be32(r.p);
    Buf o = mbuf(c * 32);

    for (DWORD i = 0; i < c; i++)
        memcpy(o.p + i * 32, r.p + row(i), 32);

    G->push(o.p, o.n);

    free(o.p);
    if (own) free(r.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:CH:COUNT", ch_count);
    h->op_name("CVM1:CH:FIRST", ch_first);
    h->op_name("CVM1:CH:HASH", ch_hash);
    h->op_name("CVM1:CH:SCORE", ch_score);
    h->op_name("CVM1:CH:ROW", ch_row);
    h->op_name("CVM1:CH:HASHES", ch_hashes);
}
