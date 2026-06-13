#include "cvm_mod.h"

static Host *G;

static uint32_t arg32(u8 *d, uint32_t n) {
    if (n >= 4) return rd32(d);

    Buf a = G->pop();
    uint32_t x = rd32(a.p);
    free(a.p);
    return x;
}

static int flow_jmp(u8 *d, uint32_t n) {
    G->cur->off = arg32(d, n);
    return 1;
}

static int flow_jrel(u8 *d, uint32_t n) {
    G->cur->off += (int32_t)arg32(d, n);
    return 1;
}

static int flow_jz(u8 *d, uint32_t n) {
    uint32_t dst = arg32(d, n);
    Buf c = G->pop();

    if (!rd32(c.p)) G->cur->off = dst;
    else G->adv();

    free(c.p);
    return 1;
}

static int flow_jnz(u8 *d, uint32_t n) {
    uint32_t dst = arg32(d, n);
    Buf c = G->pop();

    if (rd32(c.p)) G->cur->off = dst;
    else G->adv();

    free(c.p);
    return 1;
}

static int flow_next(u8 *d, uint32_t n) {
    G->adv();
    return 1;
}

static int flow_end(u8 *d, uint32_t n) {
    G->cur->off = G->cur->f.n;
    return 1;
}

static int cur_file(u8 *d, uint32_t n) {
    G->push(G->cur->f.p, G->cur->f.n);
    return 0;
}

static int cur_key(u8 *d, uint32_t n) {
    G->push(G->cur->key, 32);
    return 0;
}

static int cur_off(u8 *d, uint32_t n) {
    u8 o[4];
    wr32(o, G->cur->off);
    G->push(o, 4);
    return 0;
}

static int cur_setoff(u8 *d, uint32_t n) {
    G->cur->off = arg32(d, n);
    return 1;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:FLOW:JMP", flow_jmp);
    h->op_name("CVM1:FLOW:JREL", flow_jrel);
    h->op_name("CVM1:FLOW:JZ", flow_jz);
    h->op_name("CVM1:FLOW:JNZ", flow_jnz);
    h->op_name("CVM1:FLOW:NEXT", flow_next);
    h->op_name("CVM1:FLOW:END", flow_end);

    h->op_name("CVM1:CUR:FILE", cur_file);
    h->op_name("CVM1:CUR:KEY", cur_key);
    h->op_name("CVM1:CUR:OFF", cur_off);
    h->op_name("CVM1:CUR:SETOFF", cur_setoff);
}
