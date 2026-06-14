// 00_core: stack, bytes, u32, hash, token operations
// Compile: gcc -shared mods_src\00_core.c -Os -s -o mods\00_core.dll -lbcrypt

#include <windows.h>
#include <bcrypt.h>
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

static Host *h;

static uint32_t u32(u8 *p) {
    uint32_t x = 0;
    memcpy(&x, p, 4);
    return x;
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
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static void op_push(u8 *d, uint32_t n) {
    h->push(d, n);
    h->adv();
}

static void op_pop(u8 *d, uint32_t n) {
    Buf a = h->pop();
    free(a.p);
    h->adv();
}

static void op_dup(u8 *d, uint32_t n) {
    Buf *a = h->top();
    if (a) h->push(a->p, a->n);
    h->adv();
}

static void op_swap(u8 *d, uint32_t n) {
    Buf a = h->pop();
    Buf b = h->pop();

    h->push(a.p, a.n);
    h->push(b.p, b.n);

    free(a.p);
    free(b.p);

    h->adv();
}

static void op_len(u8 *d, uint32_t n) {
    Buf a = h->pop();
    push32(a.n);
    free(a.p);
    h->adv();
}

static void op_cat(u8 *d, uint32_t n) {
    Buf b = h->pop();
    Buf a = h->pop();

    u8 *p = malloc(a.n + b.n);

    if (a.n) memcpy(p, a.p, a.n);
    if (b.n) memcpy(p + a.n, b.p, b.n);

    h->push(p, a.n + b.n);

    free(p);
    free(a.p);
    free(b.p);

    h->adv();
}

static void op_slice(u8 *d, uint32_t n) {
    uint32_t len = pop32();
    uint32_t off = pop32();
    Buf a = h->pop();

    if (off > a.n) off = a.n;
    if (len > a.n - off) len = a.n - off;

    h->push(a.p + off, len);

    free(a.p);
    h->adv();
}

static void op_cmp(u8 *d, uint32_t n) {
    Buf b = h->pop();
    Buf a = h->pop();

    DWORD m = a.n < b.n ? a.n : b.n;
    int c = memcmp(a.p, b.p, m);

    int32_t r;
    if (c < 0) r = -1;
    else if (c > 0) r = 1;
    else if (a.n < b.n) r = -1;
    else if (a.n > b.n) r = 1;
    else r = 0;

    h->push((u8 *)&r, 4);

    free(a.p);
    free(b.p);

    h->adv();
}

static void op_take32(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 z[32] = {0};

    if (a.n)
        memcpy(z, a.p, a.n > 32 ? 32 : a.n);

    h->push(z, 32);

    free(a.p);
    h->adv();
}

static void op_hex(u8 *d, uint32_t n) {
    static char hx[] = "0123456789abcdef";

    Buf a = h->pop();
    u8 *o = malloc(a.n * 2);

    for (DWORD i = 0; i < a.n; i++) {
        o[i * 2] = hx[a.p[i] >> 4];
        o[i * 2 + 1] = hx[a.p[i] & 15];
    }

    h->push(o, a.n * 2);

    free(o);
    free(a.p);

    h->adv();
}

static int nyb(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static void op_unhex(u8 *d, uint32_t n) {
    Buf a = h->pop();
    DWORD m = a.n / 2;
    u8 *o = malloc(m);

    for (DWORD i = 0; i < m; i++)
        o[i] = (nyb(a.p[i * 2]) << 4) | nyb(a.p[i * 2 + 1]);

    h->push(o, m);

    free(o);
    free(a.p);

    h->adv();
}

static void op_const(u8 *d, uint32_t n) {
    u8 z[4] = {0};
    if (n >= 4) memcpy(z, d, 4);
    h->push(z, 4);
    h->adv();
}

#define BIN(name, expr)                         \
static void name(u8 *d, uint32_t n) {           \
    uint32_t b = pop32();                       \
    uint32_t a = pop32();                       \
    uint32_t r = (expr);                        \
    push32(r);                                  \
    h->adv();                                   \
}

BIN(op_add, a + b)
BIN(op_sub, a - b)
BIN(op_mul, a * b)
BIN(op_div, b ? a / b : 0)
BIN(op_mod, b ? a % b : 0)
BIN(op_eq,  a == b)
BIN(op_lt,  a < b)
BIN(op_gt,  a > b)
BIN(op_and, a & b)
BIN(op_or,  a | b)
BIN(op_xor, a ^ b)
BIN(op_shl, a << (b & 31))
BIN(op_shr, a >> (b & 31))

static void op_not(u8 *d, uint32_t n) {
    push32(~pop32());
    h->adv();
}

static void op_frombe(u8 *d, uint32_t n) {
    Buf a = h->pop();
    uint32_t x = 0;

    if (a.n >= 4)
        x = ((uint32_t)a.p[0] << 24) |
            ((uint32_t)a.p[1] << 16) |
            ((uint32_t)a.p[2] << 8) |
            ((uint32_t)a.p[3]);

    push32(x);

    free(a.p);
    h->adv();
}

static void op_tobe(u8 *d, uint32_t n) {
    uint32_t x = pop32();
    u8 b[4];

    b[0] = x >> 24;
    b[1] = x >> 16;
    b[2] = x >> 8;
    b[3] = x;

    h->push(b, 4);
    h->adv();
}

static void op_dec(u8 *d, uint32_t n) {
    uint32_t x = pop32();
    char s[16];
    char t[16];
    int i = 0;
    int j = 0;

    if (!x) {
        s[i++] = '0';
    } else {
        while (x) {
            t[j++] = '0' + x % 10;
            x /= 10;
        }

        while (j)
            s[i++] = t[--j];
    }

    h->push((u8 *)s, i);
    h->adv();
}

static void op_sha256(u8 *d, uint32_t n) {
    Buf a = h->pop();
    u8 out[32];

    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    DWORD objn, got;
    u8 *obj;

    BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, 0, 0);
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objn, sizeof objn, &got, 0);

    obj = malloc(objn);

    BCryptCreateHash(alg, &hash, obj, objn, 0, 0, 0);
    BCryptHashData(hash, a.p, a.n, 0);
    BCryptFinishHash(hash, out, 32, 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);

    free(obj);

    h->push(out, 32);

    free(a.p);
    h->adv();
}

static void op_tok_zero(u8 *d, uint32_t n) {
    u8 z[32] = {0};
    h->push(z, 32);
    h->adv();
}

static void op_tok_make(u8 *d, uint32_t n) {
    op_take32(d, n);
}

static void op_tok_text(u8 *d, uint32_t n) {
    Buf a = h->pop();
    DWORD m = a.n < 32 ? a.n : 32;

    while (m && a.p[m - 1] == 0)
        m--;

    h->push(a.p, m);

    free(a.p);
    h->adv();
}

static void op_tok_iszero(u8 *d, uint32_t n) {
    Buf a = h->pop();
    uint32_t r = 1;

    for (DWORD i = 0; i < a.n && i < 32; i++)
        if (a.p[i])
            r = 0;

    push32(r);

    free(a.p);
    h->adv();
}

static void op_tok_eq(u8 *d, uint32_t n) {
    Buf b = h->pop();
    Buf a = h->pop();

    u8 aa[32] = {0};
    u8 bb[32] = {0};

    if (a.n) memcpy(aa, a.p, a.n > 32 ? 32 : a.n);
    if (b.n) memcpy(bb, b.p, b.n > 32 ? 32 : b.n);

    push32(!memcmp(aa, bb, 32));

    free(a.p);
    free(b.p);

    h->adv();
}

__declspec(dllexport)
void cvm_init(Host *host) {
    h = host;

    host->op_name("ST:PUSH", op_push);
    host->op_name("ST:POP", op_pop);
    host->op_name("ST:DUP", op_dup);
    host->op_name("ST:SWAP", op_swap);

    host->op_name("BY:LEN", op_len);
    host->op_name("BY:CAT", op_cat);
    host->op_name("BY:SLICE", op_slice);
    host->op_name("BY:CMP", op_cmp);
    host->op_name("BY:TAKE32", op_take32);
    host->op_name("BY:HEX", op_hex);
    host->op_name("BY:UNHEX", op_unhex);

    host->op_name("U32:CONST", op_const);
    host->op_name("U32:ADD", op_add);
    host->op_name("U32:SUB", op_sub);
    host->op_name("U32:MUL", op_mul);
    host->op_name("U32:DIV", op_div);
    host->op_name("U32:MOD", op_mod);
    host->op_name("U32:EQ", op_eq);
    host->op_name("U32:LT", op_lt);
    host->op_name("U32:GT", op_gt);
    host->op_name("U32:AND", op_and);
    host->op_name("U32:OR", op_or);
    host->op_name("U32:XOR", op_xor);
    host->op_name("U32:SHL", op_shl);
    host->op_name("U32:SHR", op_shr);
    host->op_name("U32:NOT", op_not);
    host->op_name("U32:FROMBE", op_frombe);
    host->op_name("U32:TOBE", op_tobe);
    host->op_name("U32:DEC", op_dec);

    host->op_name("HASH:SHA256", op_sha256);

    host->op_name("TOK:ZERO", op_tok_zero);
    host->op_name("TOK:MAKE", op_tok_make);
    host->op_name("TOK:TEXT", op_tok_text);
    host->op_name("TOK:ISZERO", op_tok_iszero);
    host->op_name("TOK:EQ", op_tok_eq);
}