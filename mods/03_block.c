#include "cvm_mod.h"

static Host *G;

static int z(u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

static DWORD itemoff(u8 *b, DWORD idx) {
    DWORD o = 0;
    for (DWORD i = 0; i < idx; i++)
        o += 32 + rd32(b + o + 32);
    return o;
}

static DWORD itemlen(u8 *b, DWORD o) {
    return 32 + rd32(b + o + 32);
}

static int blk_count(u8 *d, uint32_t n) {
    Buf b = G->pop();
    DWORD o = 0, c = 0;
    while (!z(b.p + o)) {
        o += itemlen(b.p, o);
        c++;
    }
    u8 out[4];
    wr32(out, c);
    G->push(out, 4);
    free(b.p);
    return 0;
}

static int blk_hash(u8 *d, uint32_t n) {
    Buf idx = G->pop(), b = G->pop();
    DWORD o = itemoff(b.p, rd32(idx.p));
    G->push(b.p + o, 32);
    free(b.p); free(idx.p);
    return 0;
}

static int blk_data(u8 *d, uint32_t n) {
    Buf idx = G->pop(), b = G->pop();
    DWORD o = itemoff(b.p, rd32(idx.p));
    DWORD span = rd32(b.p + o + 32);
    G->push(b.p + o + 36, span - 4);
    free(b.p); free(idx.p);
    return 0;
}

static int blk_item(u8 *d, uint32_t n) {
    u8 key[32];
    u8 *data;
    DWORD len;

    if (n >= 32) {
        memcpy(key, d, 32);
        data = d + 32;
        len = n - 32;
    } else {
        Buf bd = G->pop(), bk = G->pop();
        memcpy(key, bk.p, 32);
        data = bd.p;
        len = bd.n;

        Buf o = mbuf(36 + len);
        memcpy(o.p, key, 32);
        wr32(o.p + 32, len + 4);
        memcpy(o.p + 36, data, len);
        G->push(o.p, o.n);

        free(bd.p); free(bk.p); free(o.p);
        return 0;
    }

    Buf o = mbuf(36 + len);
    memcpy(o.p, key, 32);
    wr32(o.p + 32, len + 4);
    memcpy(o.p + 36, data, len);
    G->push(o.p, o.n);
    free(o.p);
    return 0;
}

static int blk_end(u8 *d, uint32_t n) {
    u8 z[32] = {0};
    G->push(z, 32);
    return 0;
}

static int blk_ins(u8 *d, uint32_t n) {
    Buf item = G->pop(), idx = G->pop(), b = G->pop();
    DWORD o = itemoff(b.p, rd32(idx.p));
    Buf out = mbuf(b.n + item.n);

    memcpy(out.p, b.p, o);
    memcpy(out.p + o, item.p, item.n);
    memcpy(out.p + o + item.n, b.p + o, b.n - o);

    G->push(out.p, out.n);
    free(b.p); free(idx.p); free(item.p); free(out.p);
    return 0;
}

static int blk_del(u8 *d, uint32_t n) {
    Buf idx = G->pop(), b = G->pop();
    DWORD o = itemoff(b.p, rd32(idx.p));
    DWORD l = itemlen(b.p, o);
    Buf out = mbuf(b.n - l);

    memcpy(out.p, b.p, o);
    memcpy(out.p + o, b.p + o + l, b.n - o - l);

    G->push(out.p, out.n);
    free(b.p); free(idx.p); free(out.p);
    return 0;
}

static int blk_set(u8 *d, uint32_t n) {
    Buf item = G->pop(), idx = G->pop(), b = G->pop();
    DWORD o = itemoff(b.p, rd32(idx.p));
    DWORD l = itemlen(b.p, o);
    Buf out = mbuf(b.n - l + item.n);

    memcpy(out.p, b.p, o);
    memcpy(out.p + o, item.p, item.n);
    memcpy(out.p + o + item.n, b.p + o + l, b.n - o - l);

    G->push(out.p, out.n);
    free(b.p); free(idx.p); free(item.p); free(out.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:BLK:COUNT", blk_count);
    h->op_name("CVM1:BLK:HASH", blk_hash);
    h->op_name("CVM1:BLK:DATA", blk_data);
    h->op_name("CVM1:BLK:ITEM", blk_item);
    h->op_name("CVM1:BLK:END", blk_end);
    h->op_name("CVM1:BLK:INS", blk_ins);
    h->op_name("CVM1:BLK:DEL", blk_del);
    h->op_name("CVM1:BLK:SET", blk_set);
}
