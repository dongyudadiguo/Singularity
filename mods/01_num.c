#include "cvm_mod.h"

static Host *G;

static void push32(uint32_t x) {
    u8 o[4];
    wr32(o, x);
    G->push(o, 4);
}

#define BIN(name, expr) \
static int name(u8 *d, uint32_t n) { \
    Buf b = G->pop(), a = G->pop(); \
    push32(expr); \
    free(a.p); free(b.p); \
    return 0; \
}

static int u32_const(u8 *d, uint32_t n) { G->push(d, 4); return 0; }

BIN(u32_add, rd32(a.p) + rd32(b.p))
BIN(u32_sub, rd32(a.p) - rd32(b.p))
BIN(u32_mul, rd32(a.p) * rd32(b.p))
BIN(u32_div, rd32(a.p) / rd32(b.p))
BIN(u32_mod, rd32(a.p) % rd32(b.p))

BIN(u32_eq,  rd32(a.p) == rd32(b.p))
BIN(u32_lt,  rd32(a.p) <  rd32(b.p))
BIN(u32_gt,  rd32(a.p) >  rd32(b.p))

BIN(u32_and, rd32(a.p) & rd32(b.p))
BIN(u32_or,  rd32(a.p) | rd32(b.p))
BIN(u32_xor, rd32(a.p) ^ rd32(b.p))
BIN(u32_shl, rd32(a.p) << (rd32(b.p) & 31))
BIN(u32_shr, rd32(a.p) >> (rd32(b.p) & 31))

static int u32_not(u8 *d, uint32_t n) {
    Buf a = G->pop();
    push32(~rd32(a.p));
    free(a.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:U32:CONST", u32_const);
    h->op_name("CVM1:U32:ADD", u32_add);
    h->op_name("CVM1:U32:SUB", u32_sub);
    h->op_name("CVM1:U32:MUL", u32_mul);
    h->op_name("CVM1:U32:DIV", u32_div);
    h->op_name("CVM1:U32:MOD", u32_mod);

    h->op_name("CVM1:U32:EQ", u32_eq);
    h->op_name("CVM1:U32:LT", u32_lt);
    h->op_name("CVM1:U32:GT", u32_gt);

    h->op_name("CVM1:U32:AND", u32_and);
    h->op_name("CVM1:U32:OR", u32_or);
    h->op_name("CVM1:U32:XOR", u32_xor);
    h->op_name("CVM1:U32:SHL", u32_shl);
    h->op_name("CVM1:U32:SHR", u32_shr);
    h->op_name("CVM1:U32:NOT", u32_not);
}
