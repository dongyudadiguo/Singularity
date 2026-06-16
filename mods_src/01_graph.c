// 01_graph — 图操作（上传、边、投票等）
// gcc -shared mods_src/01_graph.c -Os -s -o mods/01_graph.dll

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

typedef struct {
    void (*op)(u8 *, void (*)(u8 *, uint32_t));
    void (*op_name)(char *, void (*)(u8 *, uint32_t));
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, uint32_t);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, uint32_t);
    void (*run)(u8 *);
    void (*enter)(u8 *);
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    void *cur;
} Host;

static Host *h;

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static void WU(u8 *p, uint32_t v) { memcpy(p, &v, 4); }

static u8 *dupbuf(u8 *p, uint32_t n) { u8 *b=malloc(n);if(b&&n)memcpy(b,p,n);return b; }

// RPC opcodes (must match server.go)
#define OP_REGISTER  1
#define OP_UPLOAD    2
#define OP_FILE      3
#define OP_EDGE      4
#define OP_CHILDREN  5
#define OP_VOTE      6
#define OP_USET      7
#define OP_UGET      8

static uint32_t pop_u32(void) { Buf b=h->pop(); return b.n>=4?U(b.p):0; }
static void push_u32(uint32_t v) { u8 buf[4];WU(buf,v); h->push(buf,4); }

// G: graph operations
static void g_register(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_REGISTER, p, n);
    h->push(r.p, r.n);
}

static void g_upload(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_UPLOAD, p, n);
    h->push(r.p, r.n);
}

static void g_file(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_FILE, p, n);
    h->push(r.p, r.n);
}

static void g_childs(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_CHILDREN, p, n);
    h->push(r.p, r.n);
}

static void g_child0(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 36) h->push(r.p + 4, H);
    else { u8 z[H]; memset(z,0,H); h->push(z,H); }
}

static void g_edge(u8 *p, uint32_t n) {
    h->rpc(OP_EDGE, p, n);
}

static void g_vote(u8 *p, uint32_t n) {
    h->rpc(OP_VOTE, p, n);
}

static void g_uget(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_UGET, p, n);
    h->push(r.p, r.n);
}

static void g_uset(u8 *p, uint32_t n) {
    h->rpc(OP_USET, p, n);
}

// CH: children operations
static void ch_count(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_CHILDREN, p, n);
    uint32_t cnt = 0;
    if (r.p && r.n >= 4) cnt = U(r.p);
    push_u32(cnt);
}

static void ch_first(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 36) h->push(r.p + 4, H);
    else { u8 z[H]; memset(z,0,H); h->push(z,H); }
}

static void ch_hash(u8 *p, uint32_t n) {
    // pop index, then parent hash, return child hash at index
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            h->push(r.p + 4 + idx*36, H);
            return;
        }
    }
    u8 z[H]; memset(z,0,H); h->push(z,H);
}

static void ch_score(u8 *p, uint32_t n) {
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            u8 *row = r.p + 4 + idx*36;
            int64_t score = 0;
            for (int i = 0; i < 4; i++) score = (score << 8) | row[H + i];
            push_u32((uint32_t)score);
            return;
        }
    }
    push_u32(0);
}

static void ch_row(u8 *p, uint32_t n) {
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            h->push(r.p + 4 + idx*36, 36);
            return;
        }
    }
    u8 z[36]; memset(z,0,36); h->push(z,36);
}

static void ch_hashes(u8 *p, uint32_t n) {
    Buf r = h->rpc(OP_CHILDREN, p, n);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        uint32_t total = cnt * H;
        if (4 + total <= r.n) {
            h->push(r.p + 4, total);
            return;
        }
    }
    h->push(0, 0);
}

void cvm_init(Host *host) {
    h = host;
    h->op_name("G:REGISTER", g_register);
    h->op_name("G:UPLOAD", g_upload);
    h->op_name("G:FILE", g_file);
    h->op_name("G:CHILDS", g_childs);
    h->op_name("G:CHILD0", g_child0);
    h->op_name("G:EDGE", g_edge);
    h->op_name("G:VOTE", g_vote);
    h->op_name("G:UGET", g_uget);
    h->op_name("G:USET", g_uset);
    h->op_name("CH:COUNT", ch_count);
    h->op_name("CH:FIRST", ch_first);
    h->op_name("CH:HASH", ch_hash);
    h->op_name("CH:SCORE", ch_score);
    h->op_name("CH:ROW", ch_row);
    h->op_name("CH:HASHES", ch_hashes);
}