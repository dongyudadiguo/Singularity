// 00_core — 栈、字节、数、hash、token
// gcc -shared mods_src/00_core.c -Os -s -o mods/00_core.dll -lbcrypt

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <bcrypt.h>

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

// ST: stack operations
static void st_push(void) {
    h->push(h->pay, h->plen);
    h->next();
}

static void st_pop(void) {
    Buf b = h->pop();
    if (b.p) h->push(b.p, b.n);
    h->next();
}

static void st_dup(void) {
    Buf *t = h->top();
    if (t && t->p) h->push(t->p, t->n);
    h->next();
}

static void st_swap(void) {
    Buf b1 = h->pop();
    Buf b2 = h->pop();
    if (b1.p) h->push(b1.p, b1.n);
    if (b2.p) h->push(b2.p, b2.n);
    h->next();
}

// BY: byte operations
static void by_len(void) {
    Buf b = h->pop();
    uint32_t len = b.n;
    u8 buf[4]; WU(buf, len);
    h->push(buf, 4);
    h->next();
}

static void by_cat(void) {
    Buf b2 = h->pop();
    Buf b1 = h->pop();
    uint32_t total = b1.n + b2.n;
    u8 *buf = malloc(total);
    if (b1.n) memcpy(buf, b1.p, b1.n);
    if (b2.n) memcpy(buf + b1.n, b2.p, b2.n);
    h->push(buf, total);
    free(buf);
    h->next();
}

static void by_slice(void) {
    if (h->plen < 8) { h->next(); return; }
    uint32_t off = U(h->pay);
    uint32_t len = U(h->pay + 4);
    Buf b = h->pop();
    if (off >= b.n) { h->push(0, 0); h->next(); return; }
    if (off + len > b.n) len = b.n - off;
    h->push(b.p + off, len);
    h->next();
}

static void by_cmp(void) {
    Buf b2 = h->pop();
    Buf b1 = h->pop();
    uint32_t mn = b1.n < b2.n ? b1.n : b2.n;
    int r = memcmp(b1.p, b2.p, mn);
    if (!r) r = (int)b1.n - (int)b2.n;
    u8 buf[4]; WU(buf, r < 0 ? 1 : r > 0 ? 2 : 0);
    h->push(buf, 4);
    h->next();
}

static void by_take32(void) {
    Buf b = h->pop();
    if (b.n >= H) h->push(b.p, H);
    else { u8 buf[H]; memset(buf, 0, H); memcpy(buf, b.p, b.n); h->push(buf, H); }
    h->next();
}

static void by_hex(void) {
    Buf b = h->pop();
    u8 *buf = malloc(b.n * 2);
    for (uint32_t i = 0; i < b.n; i++) {
        buf[i*2] = "0123456789abcdef"[b.p[i] >> 4];
        buf[i*2+1] = "0123456789abcdef"[b.p[i] & 0xf];
    }
    h->push(buf, b.n * 2);
    free(buf);
    h->next();
}

static void by_unhex(void) {
    Buf b = h->pop();
    u8 *buf = malloc(b.n / 2);
    for (uint32_t i = 0; i < b.n / 2; i++) {
        int hi = b.p[i*2], lo = b.p[i*2+1];
        hi = (hi >= 'a' ? hi - 'a' + 10 : hi >= 'A' ? hi - 'A' + 10 : hi - '0');
        lo = (lo >= 'a' ? lo - 'a' + 10 : lo >= 'A' ? lo - 'A' + 10 : lo - '0');
        buf[i] = (hi << 4) | lo;
    }
    h->push(buf, b.n / 2);
    free(buf);
    h->next();
}

// U32: 32-bit unsigned integer operations
static void u32_const(void) {
    h->push(h->pay, 4);
    h->next();
}

static uint32_t pop_u32(void) {
    Buf b = h->pop();
    return b.n >= 4 ? U(b.p) : 0;
}

static void push_u32(uint32_t v) {
    u8 buf[4]; WU(buf, v);
    h->push(buf, 4);
}

static void u32_add(void) { push_u32(pop_u32() + pop_u32()); h->next(); }
static void u32_sub(void) { uint32_t b = pop_u32(); uint32_t a = pop_u32(); push_u32(a - b); h->next(); }
static void u32_mul(void) { push_u32(pop_u32() * pop_u32()); h->next(); }
static void u32_div(void) { uint32_t b = pop_u32(); uint32_t a = pop_u32(); push_u32(b ? a / b : 0); h->next(); }
static void u32_mod(void) { uint32_t b = pop_u32(); uint32_t a = pop_u32(); push_u32(b ? a % b : 0); h->next(); }
static void u32_eq(void) { push_u32(pop_u32() == pop_u32() ? 1 : 0); h->next(); }
static void u32_lt(void) { uint32_t b = pop_u32(); uint32_t a = pop_u32(); push_u32(a < b ? 1 : 0); h->next(); }
static void u32_gt(void) { uint32_t b = pop_u32(); uint32_t a = pop_u32(); push_u32(a > b ? 1 : 0); h->next(); }
static void u32_and(void) { push_u32(pop_u32() & pop_u32()); h->next(); }
static void u32_or(void) { push_u32(pop_u32() | pop_u32()); h->next(); }
static void u32_xor(void) { push_u32(pop_u32() ^ pop_u32()); h->next(); }
static void u32_shl(void) { uint32_t s = pop_u32(); uint32_t v = pop_u32(); push_u32(v << s); h->next(); }
static void u32_shr(void) { uint32_t s = pop_u32(); uint32_t v = pop_u32(); push_u32(v >> s); h->next(); }
static void u32_not(void) { push_u32(~pop_u32()); h->next(); }
static void u32_frombe(void) { Buf b = h->pop(); if(b.n>=4){uint32_t v=((uint32_t)b.p[0]<<24)|((uint32_t)b.p[1]<<16)|((uint32_t)b.p[2]<<8)|b.p[3];push_u32(v);} h->next(); }
static void u32_tobe(void) { uint32_t v=pop_u32();u8 buf[4];buf[0]=v>>24;buf[1]=v>>16;buf[2]=v>>8;buf[3]=v;h->push(buf,4); h->next(); }
static void u32_dec(void) { uint32_t v=pop_u32();char buf[16];int len=snprintf(buf,sizeof buf,"%u",v);h->push((u8*)buf,len); h->next(); }

// HASH: sha256
static void hash_sha256(void) {
    Buf b = h->pop();
    u8 hash[H];
    BCRYPT_ALG_HANDLE  alg = NULL;
    BCRYPT_HASH_HANDLE hh  = NULL;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0) {
        if (BCryptCreateHash(alg, &hh, NULL, 0, NULL, 0, 0) == 0) {
            BCryptHashData(hh, b.p, b.n, 0);
            BCryptFinishHash(hh, hash, H, 0);
            BCryptDestroyHash(hh);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    h->push(hash, H);
    h->next();
}

// TOK: token operations
static void tok_zero(void) { u8 z[H]; memset(z,0,H); h->push(z,H); h->next(); }
static void tok_make(void) { Buf b=h->pop(); u8 t[H]; memset(t,0,H); memcpy(t,b.p,b.n<H?b.n:H); h->push(t,H); h->next(); }
static void tok_text(void) { Buf b=h->pop(); h->push(b.p,b.n); h->next(); }
static void tok_iszero(void) { Buf b=h->pop(); int z=1; for(uint32_t i=0;i<b.n&&i<H;i++)if(b.p[i])z=0; push_u32(z); h->next(); }
static void tok_eq(void) { Buf b2=h->pop();Buf b1=h->pop();push_u32(b1.n==b2.n&&!memcmp(b1.p,b2.p,b1.n)?1:0); h->next(); }

void cvm_init(Host *host) {
    h = host;
    h->op_name("ST:PUSH", st_push);
    h->op_name("ST:POP", st_pop);
    h->op_name("ST:DUP", st_dup);
    h->op_name("ST:SWAP", st_swap);
    h->op_name("BY:LEN", by_len);
    h->op_name("BY:CAT", by_cat);
    h->op_name("BY:SLICE", by_slice);
    h->op_name("BY:CMP", by_cmp);
    h->op_name("BY:TAKE32", by_take32);
    h->op_name("BY:HEX", by_hex);
    h->op_name("BY:UNHEX", by_unhex);
    h->op_name("U32:CONST", u32_const);
    h->op_name("U32:ADD", u32_add);
    h->op_name("U32:SUB", u32_sub);
    h->op_name("U32:MUL", u32_mul);
    h->op_name("U32:DIV", u32_div);
    h->op_name("U32:MOD", u32_mod);
    h->op_name("U32:EQ", u32_eq);
    h->op_name("U32:LT", u32_lt);
    h->op_name("U32:GT", u32_gt);
    h->op_name("U32:AND", u32_and);
    h->op_name("U32:OR", u32_or);
    h->op_name("U32:XOR", u32_xor);
    h->op_name("U32:SHL", u32_shl);
    h->op_name("U32:SHR", u32_shr);
    h->op_name("U32:NOT", u32_not);
    h->op_name("U32:FROMBE", u32_frombe);
    h->op_name("U32:TOBE", u32_tobe);
    h->op_name("U32:DEC", u32_dec);
    h->op_name("HASH:SHA256", hash_sha256);
    h->op_name("TOK:ZERO", tok_zero);
    h->op_name("TOK:MAKE", tok_make);
    h->op_name("TOK:TEXT", tok_text);
    h->op_name("TOK:ISZERO", tok_iszero);
    h->op_name("TOK:EQ", tok_eq);
}