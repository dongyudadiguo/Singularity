// 01_graph: graph operations (register, upload, file, edge, children, vote, uset, uget)

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define H 32

typedef unsigned char u8;

typedef struct {
    u8 *p;
    DWORD n;
} Buf;

typedef struct {
    Buf f;
    DWORD off;
    u8 key[H];
} Frame;

typedef void (*Op)(u8 *data, uint32_t len);

typedef struct Host {
    void (*op)(u8 *id, Op fn);
    void (*op_name)(char *name, Op fn);
    void (*del)(u8 *id);
    void (*del_name)(char *name);

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf  (*rpc)(uint8_t op, u8 *body, DWORD len);

    void (*run)(u8 *hash);
    void (*enter)(u8 *hash);
    void (*adv)();

    void (*push)(u8 *p, DWORD n);
    Buf  (*pop)();
    Buf *(*top)();

    Frame *cur;
} Host;

#define OP_REGISTER 1
#define OP_UPLOAD   2
#define OP_FILE     3
#define OP_EDGE     4
#define OP_CHILDREN 5
#define OP_VOTE     6
#define OP_USET     7
#define OP_UGET     8

static Host *h;

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
    h->push(b, 4);
}

static uint32_t pop32() {
    Buf b = h->pop();
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
    Buf a = h->pop();
    Buf r = h->rpc(OP_REGISTER, a.p, a.n);

    h->push(r.p, r.n);

    free(a.p);
    free(r.p);

    h->adv();
}

static void op_upload(u8 *d, uint32_t n) {
    Buf a = h->pop();
    Buf r = h->rpc(OP_UPLOAD, a.p, a.n);

    h->push(r.p, r.n);

    free(a.p);
    free(r.p);

    h->adv();
}

static void op_file(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 k[32];

    take32(&a, k);

    Buf r = h->rpc(OP_FILE, k, 32);

    h->push(r.p, r.n);

    free(a.p);
    free(r.p);

    h->adv();
}

static void op_childs(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 k[32];

    take32(&a, k);

    Buf r = h->rpc(OP_CHILDREN, k, 32);

    h->push(r.p, r.n);

    free(a.p);
    free(r.p);

    h->adv();
}

static void op_child0(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 k[32];
    u8 z[32] = {0};

    take32(&a, k);

    Buf r = h->rpc(OP_CHILDREN, k, 32);

    if (r.n >= 36 && be32(r.p) > 0)
        h->push(r.p + 4, 32);
    else
        h->push(z, 32);

    free(a.p);
    free(r.p);

    h->adv();
}

static void op_edge(u8 *d, uint32_t n) {
    Buf child = h->pop();
    Buf parent = h->pop();
    u8 body[64] = {0};

    memcpy(body, parent.p, parent.n > 32 ? 32 : parent.n);
    memcpy(body + 32, child.p, child.n > 32 ? 32 : child.n);

    Buf r = h->rpc(OP_EDGE, body, 64);

    free(parent.p);
    free(child.p);
    free(r.p);

    h->adv();
}

static void op_vote(u8 *d, uint32_t n) {
    Buf child = h->pop();
    Buf parent = h->pop();
    Buf user = h->pop();
    u8 body[96] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, parent.p, parent.n > 32 ? 32 : parent.n);
    memcpy(body + 64, child.p, child.n > 32 ? 32 : child.n);

    Buf r = h->rpc(OP_VOTE, body, 96);

    free(user.p);
    free(parent.p);
    free(child.p);
    free(r.p);

    h->adv();
}

static void op_uset(u8 *d, uint32_t n) {
    Buf val = h->pop();
    Buf key = h->pop();
    Buf user = h->pop();
    u8 body[96] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, key.p, key.n > 32 ? 32 : key.n);
    memcpy(body + 64, val.p, val.n > 32 ? 32 : val.n);

    Buf r = h->rpc(OP_USET, body, 96);

    free(user.p);
    free(key.p);
    free(val.p);
    free(r.p);

    h->adv();
}

static void op_uget(u8 *d, uint32_t n) {
    Buf key = h->pop();
    Buf user = h->pop();
    u8 body[64] = {0};
    u8 z[32] = {0};

    memcpy(body, user.p, user.n > 32 ? 32 : user.n);
    memcpy(body + 32, key.p, key.n > 32 ? 32 : key.n);

    Buf r = h->rpc(OP_UGET, body, 64);

    if (r.n == 32)
        h->push(r.p, 32);
    else
        h->push(z, 32);

    free(user.p);
    free(key.p);
    free(r.p);

    h->adv();
}

static void op_ch_count(u8 *d, uint32_t n) {
    Buf a = h->pop();

    push32(a.n >= 4 ? be32(a.p) : 0);

    free(a.p);
    h->adv();
}

static void op_ch_first(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 z[32] = {0};

    if (a.n >= 36 && be32(a.p) > 0)
        h->push(a.p + 4, 32);
    else
        h->push(z, 32);

    free(a.p);
    h->adv();
}

static void op_ch_hash(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = h->pop();
    u8 z[32] = {0};

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40;

    if (idx < cnt && off + 32 <= a.n)
        h->push(a.p + off, 32);
    else
        h->push(z, 32);

    free(a.p);
    h->adv();
}

static void op_ch_score(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = h->pop();
    u8 z[8] = {0};

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40 + 32;

    if (idx < cnt && off + 8 <= a.n) {
        for (int i = 0; i < 8; i++)
            z[i] = a.p[off + 7 - i];
    }

    h->push(z, 8);

    free(a.p);
    h->adv();
}

static void op_ch_row(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf a = h->pop();

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    DWORD off = 4 + idx * 40;

    if (idx < cnt && off + 40 <= a.n)
        h->push(a.p + off, 40);
    else
        h->push(0, 0);

    free(a.p);
    h->adv();
}

static void op_ch_hashes(u8 *d, uint32_t n) {
    Buf a = h->pop();

    uint32_t cnt = a.n >= 4 ? be32(a.p) : 0;
    uint32_t max = a.n >= 4 ? (a.n - 4) / 40 : 0;

    if (cnt > max)
        cnt = max;

    u8 *o = malloc(cnt * 32);

    for (uint32_t i = 0; i < cnt; i++)
        memcpy(o + i * 32, a.p + 4 + i * 40, 32);

    h->push(o, cnt * 32);

    free(o);
    free(a.p);

    h->adv();
}

__declspec(dllexport)
void cvm_init(Host *host) {
    h = host;

    host->op_name("G:REGISTER", op_register);
    host->op_name("G:UPLOAD", op_upload);
    host->op_name("G:FILE", op_file);
    host->op_name("G:CHILDS", op_childs);
    host->op_name("G:CHILD0", op_child0);
    host->op_name("G:EDGE", op_edge);
    host->op_name("G:VOTE", op_vote);
    host->op_name("G:UGET", op_uget);
    host->op_name("G:USET", op_uset);

    host->op_name("CH:COUNT", op_ch_count);
    host->op_name("CH:FIRST", op_ch_first);
    host->op_name("CH:HASH", op_ch_hash);
    host->op_name("CH:SCORE", op_ch_score);
    host->op_name("CH:ROW", op_ch_row);
    host->op_name("CH:HASHES", op_ch_hashes);
}