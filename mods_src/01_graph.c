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
    void (*op)(u8 *, void (*)(void));
    void (*op_name)(char *, void (*)(void));
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
    u8 *pay; uint32_t plen;
    void (*next)(void);
    void (*next_noadv)(void);
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
static void g_register(void) {
    Buf r = h->rpc(OP_REGISTER, h->pay, h->plen);
    h->push(r.p, r.n);
    h->next();
}

static void g_upload(void) {
    Buf r = h->rpc(OP_UPLOAD, h->pay, h->plen);
    h->push(r.p, r.n);
    h->next();
}

static void g_file(void) {
    Buf r = h->rpc(OP_FILE, h->pay, h->plen);
    h->push(r.p, r.n);
    h->next();
}

static void g_childs(void) {
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    h->push(r.p, r.n);
    h->next();
}

static void g_child0(void) {
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 36) h->push(r.p + 4, H);
    else { u8 z[H]; memset(z,0,H); h->push(z,H); }
    h->next();
}

static void g_edge(void) {
    h->rpc(OP_EDGE, h->pay, h->plen);
    h->next();
}

static void g_vote(void) {
    h->rpc(OP_VOTE, h->pay, h->plen);
    h->next();
}

static void g_uget(void) {
    Buf r = h->rpc(OP_UGET, h->pay, h->plen);
    h->push(r.p, r.n);
    h->next();
}

static void g_uset(void) {
    h->rpc(OP_USET, h->pay, h->plen);
    h->next();
}

// CH: children operations
static void ch_count(void) {
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    uint32_t cnt = 0;
    if (r.p && r.n >= 4) cnt = U(r.p);
    push_u32(cnt);
    h->next();
}

static void ch_first(void) {
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 36) h->push(r.p + 4, H);
    else { u8 z[H]; memset(z,0,H); h->push(z,H); }
    h->next();
}

static void ch_hash(void) {
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            h->push(r.p + 4 + idx*36, H);
            h->next();
            return;
        }
    }
    u8 z[H]; memset(z,0,H); h->push(z,H);
    h->next();
}

static void ch_score(void) {
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            u8 *row = r.p + 4 + idx*36;
            int64_t score = 0;
            for (int i = 0; i < 4; i++) score = (score << 8) | row[H + i];
            push_u32((uint32_t)score);
            h->next();
            return;
        }
    }
    push_u32(0);
    h->next();
}

static void ch_row(void) {
    uint32_t idx = pop_u32();
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        if (idx < cnt && r.n >= 4 + (idx+1)*36) {
            h->push(r.p + 4 + idx*36, 36);
            h->next();
            return;
        }
    }
    u8 z[36]; memset(z,0,36); h->push(z,36);
    h->next();
}

static void ch_hashes(void) {
    Buf r = h->rpc(OP_CHILDREN, h->pay, h->plen);
    if (r.p && r.n >= 4) {
        uint32_t cnt = U(r.p);
        uint32_t total = cnt * H;
        if (4 + total <= r.n) {
            h->push(r.p + 4, total);
            h->next();
            return;
        }
    }
    h->push(0, 0);
    h->next();
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