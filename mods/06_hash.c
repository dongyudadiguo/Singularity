#define _WIN32_WINNT 0x0A00
#include "cvm_mod.h"
#include <bcrypt.h>

static Host *G;

static uint32_t rdbe(u8 *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static void wrbe(u8 *p, uint32_t x) {
    p[0] = x >> 24;
    p[1] = x >> 16;
    p[2] = x >> 8;
    p[3] = x;
}

static int hash_sha256(u8 *d, uint32_t n) {
    Buf a;
    int own = !n;
    u8 out[32];
    BCRYPT_ALG_HANDLE h;

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    BCryptOpenAlgorithmProvider(&h, BCRYPT_SHA256_ALGORITHM, 0, 0);
    BCryptHash(h, 0, 0, a.p, a.n, out, 32);
    BCryptCloseAlgorithmProvider(h, 0);

    G->push(out, 32);
    if (own) free(a.p);
    return 0;
}

static int u32_frombe(u8 *d, uint32_t n) {
    Buf a;
    u8 o[4];

    if (n >= 4) {
        wr32(o, rdbe(d));
    } else {
        a = G->pop();
        wr32(o, rdbe(a.p));
        free(a.p);
    }

    G->push(o, 4);
    return 0;
}

static int u32_tobe(u8 *d, uint32_t n) {
    Buf a;
    u8 o[4];

    if (n >= 4) {
        wrbe(o, rd32(d));
    } else {
        a = G->pop();
        wrbe(o, rd32(a.p));
        free(a.p);
    }

    G->push(o, 4);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:HASH:SHA256", hash_sha256);
    h->op_name("CVM1:U32:FROMBE", u32_frombe);
    h->op_name("CVM1:U32:TOBE", u32_tobe);
}
