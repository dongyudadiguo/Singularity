#include "cvm_mod.h"

static Host *G;

static void push32(uint32_t x) {
    u8 o[4];
    wr32(o, x);
    G->push(o, 4);
}

static int rect_make(u8 *d, uint32_t n) {
    u8 o[16];

    if (n >= 16) {
        G->push(d, 16);
        return 0;
    }

    Buf h = G->pop(), w = G->pop(), y = G->pop(), x = G->pop();

    wr32(o, rd32(x.p));
    wr32(o + 4, rd32(y.p));
    wr32(o + 8, rd32(w.p));
    wr32(o + 12, rd32(h.p));

    G->push(o, 16);

    free(x.p); free(y.p); free(w.p); free(h.p);
    return 0;
}

#define PART(name, at) \
static int name(u8 *d, uint32_t n) { \
    Buf r = G->pop(); \
    push32(rd32(r.p + at)); \
    free(r.p); \
    return 0; \
}

PART(rect_x, 0)
PART(rect_y, 4)
PART(rect_w, 8)
PART(rect_h, 12)

static int rect_pad(u8 *d, uint32_t n) {
    Buf p = G->pop(), r = G->pop();
    DWORD a = rd32(p.p);
    u8 o[16];

    wr32(o, rd32(r.p) + a);
    wr32(o + 4, rd32(r.p + 4) + a);
    wr32(o + 8, rd32(r.p + 8) - a * 2);
    wr32(o + 12, rd32(r.p + 12) - a * 2);

    G->push(o, 16);

    free(p.p);
    free(r.p);
    return 0;
}

static int rect_hit(u8 *d, uint32_t n) {
    Buf m = G->pop(), r = G->pop();
    int x = rd32(m.p), y = rd32(m.p + 4);
    int rx = rd32(r.p), ry = rd32(r.p + 4), rw = rd32(r.p + 8), rh = rd32(r.p + 12);

    push32(x >= rx && y >= ry && x < rx + rw && y < ry + rh);

    free(m.p);
    free(r.p);
    return 0;
}

static int rect_row(u8 *d, uint32_t n) {
    Buf rh = G->pop(), idx = G->pop(), r = G->pop();
    u8 o[16];
    DWORD h = rd32(rh.p);

    wr32(o, rd32(r.p));
    wr32(o + 4, rd32(r.p + 4) + rd32(idx.p) * h);
    wr32(o + 8, rd32(r.p + 8));
    wr32(o + 12, h);

    G->push(o, 16);

    free(r.p); free(idx.p); free(rh.p);
    return 0;
}

static int rect_col(u8 *d, uint32_t n) {
    Buf cw = G->pop(), idx = G->pop(), r = G->pop();
    u8 o[16];
    DWORD w = rd32(cw.p);

    wr32(o, rd32(r.p) + rd32(idx.p) * w);
    wr32(o + 4, rd32(r.p + 4));
    wr32(o + 8, w);
    wr32(o + 12, rd32(r.p + 12));

    G->push(o, 16);

    free(r.p); free(idx.p); free(cw.p);
    return 0;
}

static int rect_top(u8 *d, uint32_t n) {
    Buf h = G->pop(), r = G->pop();
    u8 o[16];

    memcpy(o, r.p, 16);
    wr32(o + 12, rd32(h.p));

    G->push(o, 16);

    free(h.p); free(r.p);
    return 0;
}

static int rect_bottom(u8 *d, uint32_t n) {
    Buf h = G->pop(), r = G->pop();
    u8 o[16];
    DWORD hh = rd32(h.p);

    wr32(o, rd32(r.p));
    wr32(o + 4, rd32(r.p + 4) + rd32(r.p + 12) - hh);
    wr32(o + 8, rd32(r.p + 8));
    wr32(o + 12, hh);

    G->push(o, 16);

    free(h.p); free(r.p);
    return 0;
}

static int rect_left(u8 *d, uint32_t n) {
    Buf w = G->pop(), r = G->pop();
    u8 o[16];

    memcpy(o, r.p, 16);
    wr32(o + 8, rd32(w.p));

    G->push(o, 16);

    free(w.p); free(r.p);
    return 0;
}

static int rect_right(u8 *d, uint32_t n) {
    Buf w = G->pop(), r = G->pop();
    u8 o[16];
    DWORD ww = rd32(w.p);

    wr32(o, rd32(r.p) + rd32(r.p + 8) - ww);
    wr32(o + 4, rd32(r.p + 4));
    wr32(o + 8, ww);
    wr32(o + 12, rd32(r.p + 12));

    G->push(o, 16);

    free(w.p); free(r.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:RECT:MAKE", rect_make);
    h->op_name("CVM1:RECT:X", rect_x);
    h->op_name("CVM1:RECT:Y", rect_y);
    h->op_name("CVM1:RECT:W", rect_w);
    h->op_name("CVM1:RECT:H", rect_h);
    h->op_name("CVM1:RECT:PAD", rect_pad);
    h->op_name("CVM1:RECT:HIT", rect_hit);
    h->op_name("CVM1:RECT:ROW", rect_row);
    h->op_name("CVM1:RECT:COL", rect_col);
    h->op_name("CVM1:RECT:TOP", rect_top);
    h->op_name("CVM1:RECT:BOTTOM", rect_bottom);
    h->op_name("CVM1:RECT:LEFT", rect_left);
    h->op_name("CVM1:RECT:RIGHT", rect_right);
}
