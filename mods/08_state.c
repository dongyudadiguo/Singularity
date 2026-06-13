#include "cvm_mod.h"

#define VN 512

static Host *G;

typedef struct {
    u8 key[32];
    Buf val;
} Var;

static Var vs[VN];
static int vn;

static Buf cp(u8 *p, DWORD n) {
    Buf b = mbuf(n);
    memcpy(b.p, p, n);
    return b;
}

static Var *find(u8 *k) {
    for (int i = vn - 1; i >= 0; i--)
        if (!memcmp(vs[i].key, k, 32)) return vs + i;
    return 0;
}

static void keyarg(u8 *d, uint32_t n, u8 k[32]) {
    if (n >= 32) memcpy(k, d, 32);
    else {
        Buf a = G->pop();
        memcpy(k, a.p, 32);
        free(a.p);
    }
}

static int var_set(u8 *d, uint32_t n) {
    u8 k[32];
    Var *v;

    if (n >= 32) {
        memcpy(k, d, 32);
        v = find(k);
        if (!v) v = vs + vn++, memcpy(v->key, k, 32);
        free(v->val.p);
        v->val = cp(d + 32, n - 32);
    } else {
        Buf val = G->pop(), key = G->pop();
        v = find(key.p);
        if (!v) v = vs + vn++, memcpy(v->key, key.p, 32);
        free(v->val.p);
        v->val = cp(val.p, val.n);
        free(key.p); free(val.p);
    }

    return 0;
}

static int var_get(u8 *d, uint32_t n) {
    u8 k[32], z = 0;
    Var *v;

    keyarg(d, n, k);
    v = find(k);

    if (v) G->push(v->val.p, v->val.n);
    else G->push(&z, 0);

    return 0;
}

static int var_has(u8 *d, uint32_t n) {
    u8 k[32], o[4];
    keyarg(d, n, k);
    wr32(o, find(k) != 0);
    G->push(o, 4);
    return 0;
}

static int var_del(u8 *d, uint32_t n) {
    u8 k[32];
    Var *v;

    keyarg(d, n, k);
    v = find(k);

    if (v) {
        free(v->val.p);
        *v = vs[--vn];
    }

    return 0;
}

static int var_clear(u8 *d, uint32_t n) {
    for (int i = 0; i < vn; i++) free(vs[i].val.p);
    vn = 0;
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:VAR:SET", var_set);
    h->op_name("CVM1:VAR:GET", var_get);
    h->op_name("CVM1:VAR:HAS", var_has);
    h->op_name("CVM1:VAR:DEL", var_del);
    h->op_name("CVM1:VAR:CLEAR", var_clear);
}
