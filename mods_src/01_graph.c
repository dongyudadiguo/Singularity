#include "../cvm_host.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define OP_REGISTER 1
#define OP_UPLOAD   2
#define OP_FILE     3
#define OP_EDGE     4
#define OP_CHILDREN 5
#define OP_VOTE     6
#define OP_USET     7
#define OP_UGET     8

static Host *H;

static uint32_t be32(u8 *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           ((uint32_t)p[3]);
}

static void put32(u8 *p, uint32_t x) {
    memcpy(p, &x, 4);
}

static void push32(uint32_t x) {
    u8 b[4];
    put32(b, x);
    H->push(b, 4);
}

static uint32_t pop32() {
    Buf b = H->pop();
    uint32_t x = 0;

    if (b.n >= 4)
        memcpy(&x, b.p, 4);

    free(b.p);
    return x;
}

static void take32(Buf *b, u8 out[32]) {
    memset(out, 0, 32);
    if (b->n)
        memcpy(out, b->p, b->n > 32 ? 32 : b->n);
}

static void op_register(u8 *d, uint32_t n) {
    Buf a = H->pop();
    Buf r = H->rpc(OP_REGISTER, a.p, a.n);

    H->push(r.p, r.n);

    free(a.p);
    free(r.p);

    H->adv();
}

static void op_upload(u8 *d, uint32_t n) {
    Buf a = H->pop();
    Buf r = H->rpc(OP_UPLOAD, a.p, a.n);

    H->push(r.p, r.n);

    free(a.p);
    free(r.p);

    H->adv();
}

static void op_file(u8 *d, uint32_t n) {
    Buf a = H->pop();
    u8 h[32];

    take32(&a, h);

    Buf r = H->rpc(OP_FILE, h, 32);

    H->push(r.p, r.n);

    free(a.p);
    free(r.p);

    H->adv();
}

static void op_childs(u8 *d, uint32_t n) {
    Buf a = H->pop();
    u8 h[32];

    take32(&a, h);

    Buf r = H->rpc(OP_CHILDREN, h, 32);

    H->push(r.p, r.n);

    free(a.p);
    free(r.p);

    H->adv();
}

static void op_child0(u8 *d, uint32_t n) {
    Buf a = H->pop();
    u8 h[32];
    u8 z[32] = {0};

    take32(&a, h);

    Buf r = H->rpc(OP_CHILDREN, h, 32);

    if (r.n >= 36 && be32(r.p) > 0)
        H->push(r.p + 4, 32);
    else
        H->push(z, 32);

    free(a.p);
    free(r.p);

    H->adv();
}

static void op_edge(u8 *d, uint32_t n) {
    Buf child = H->pop();
    Buf parent = H->pop();
    u8 body[64] = {0};

    memcpy(body, parent.p, parent.n > 32 ? 32 : parent.n);
    memcpy(body + 32, child.p, child.n > 32 ? 32 : child.n);

    Buf r = H->rpc(OP_EDGE, body, 64);

    free(parent.p);
    free(child.p);
    free(r.p);

    H->adv();
}

static void op_vote(u8 *d, uint32_t n) {
    Buf child = H->pop();
    Buf parent = H->pop();
    Buf user = H->pop();
    u8 body[96] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, parent.p, parent.n > 32 ? 32 : parent.n);
    memcpy(body + 64, child.p, child.n > 32 ? 32 : child.n);

    Buf r = H->rpc(OP_VOTE, body, 96);

    free(user.p);
    free(parent.p);
    free(child.p);
    free(r.p);

    H->adv();
}

static void op_uset(u8 *d, uint32_t n) {
    Buf val = H->pop();
    Buf key = H->pop();
    Buf user = H->pop();
    u8 body[96] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, key.p, key.n > 32 ? 32 : key.n);
    memcpy(body + 64, val.p, val.n > 32 ? 32 : val.n);

    Buf r = H->rpc(OP_USET, body, 96);

    free(user.p);
    free(key.p);
    free(val.p);
    free(r.p);

    H->adv();
}

static void op_uget(u8 *d, uint32_t n) {
    Buf key = H->pop();
    Buf user = H->pop();
    u8 body[64] = {0};
    u8 z[32] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, key.p, key.n > 32 ? 32 : key.n);

    Buf r = H->rpc(OP_UGET, body, 64);

    if (r.n == 32)
        H->push(r.p, 32);
    else
        H->push(z, 32);

    free(user.p);
    free(key.p);
    free(r.p);

    H->adv();
}

static void op_ch_count(u8 *d, uint32_t n) {
    Buf a = H->pop();

    push32(a.n >= 4 ? be32(a.p) : 0);

    free(a.p);
    H->adv();
}

static void op_ch_first(u8 *d, uint32_t n) {
    Buf a = H->pop();
    u8 z[32] = {0};

    if (a.n >= 36 && be32(a.p) > 0)
        H->push(a.p + 4, 32);
    else
        H->push(z, 32);

    free(a.p);
    H->adv();
}

static void op_ch_hash(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = H->pop();
    u8 z[32] = {0};

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40;

    if (idx < cnt && off + 32 <= a.n)
        H->push(a.p + off, 32);
    else
        H->push(z, 32);

    free(a.p);
    H->adv();
}

static void op_ch_score(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = H->pop();
    u8 z[8] = {0};

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40 + 32;

    if (idx < cnt && off + 8 <= a.n) {
        for (int i = 0; i < 8; i++)
            z[i] = a.p[off + 7 - i];
    }

    H->push(z, 8);

    free(a.p);
    H->adv();
}

static void op_ch_row(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = H->pop();

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40;

    if (idx < cnt && off + 40 <= a.n)
        H->push(a.p + off, 40);
    else
        H->push(0, 0);

    free(a.p);
    H->adv();
}

static void op_ch_hashes(u8 *d, uint32_t n) {
    Buf a = H->pop();

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    uint32_t max = a.n >= 4 ? (a.n - 4) / 40 : 0;

    if (cnt > max)
        cnt = max;

    u8 *o = malloc(cnt * 32);

    for (uint32_t i = 0; i < cnt; i++)
        memcpy(o + i * 32, a.p + 4 + i * 40, 32);

    H->push(o, cnt * 32);

    free(o);
    free(a.p);

    H->adv();
}

__declspec(dllexport)
void cvm_init(Host *h) {
    H = h;

    h->op_name("G:REGISTER", op_register);
    h->op_name("G:UPLOAD", op_upload);
    h->op_name("G:FILE", op_file);
    h->op_name("G:CHILDS", op_childs);
    h->op_name("G:CHILD0", op_child0);
    h->op_name("G:EDGE", op_edge);
    h->op_name("G:VOTE", op_vote);
    h->op_name("G:UGET", op_uget);
    h->op_name("G:USET", op_uset);

    h->op_name("CH:COUNT", op_ch_count);
    h->op_name("CH:FIRST", op_ch_first);
    h->op_name("CH:HASH", op_ch_hash);
    h->op_name("CH:SCORE", op_ch_score);
    h->op_name("CH:ROW", op_ch_row);
    h->op_name("CH:HASHES", op_ch_hashes);
}