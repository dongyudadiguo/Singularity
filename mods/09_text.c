#include "cvm_mod.h"

static Host *G;

static void push32(uint32_t x) {
    u8 o[4];
    wr32(o, x);
    G->push(o, 4);
}

static DWORD findmem(u8 *a, DWORD an, u8 *b, DWORD bn) {
    if (!bn) return 0;

    for (DWORD i = 0; i + bn <= an; i++)
        if (!memcmp(a + i, b, bn)) return i;

    return 0xffffffff;
}

static int txt_u8len(u8 *d, uint32_t n) {
    Buf a;
    DWORD c = 0;

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    for (DWORD i = 0; i < a.n; i++)
        if ((a.p[i] & 0xc0) != 0x80) c++;

    push32(c);
    if (!n) free(a.p);
    return 0;
}

static int txt_find(u8 *d, uint32_t n) {
    Buf needle = G->pop(), text = G->pop();
    push32(findmem(text.p, text.n, needle.p, needle.n));
    free(text.p); free(needle.p);
    return 0;
}

static int txt_lines(u8 *d, uint32_t n) {
    Buf a;
    DWORD c = 1;

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    for (DWORD i = 0; i < a.n; i++)
        if (a.p[i] == '\n') c++;

    push32(c);
    if (!n) free(a.p);
    return 0;
}

static int txt_line(u8 *d, uint32_t n) {
    Buf idx = G->pop(), t = G->pop();
    DWORD want = rd32(idx.p), cur = 0, s = 0, e = t.n;

    for (DWORD i = 0; i < t.n; i++) {
        if (cur == want) { s = i; break; }
        if (t.p[i] == '\n') cur++;
    }

    for (DWORD i = s; i < t.n; i++)
        if (t.p[i] == '\n') { e = i; break; }

    G->push(t.p + s, e - s);

    free(t.p); free(idx.p);
    return 0;
}

static int txt_ins(u8 *d, uint32_t n) {
    Buf ins = G->pop(), off = G->pop(), t = G->pop();
    DWORD o = rd32(off.p);
    Buf r = mbuf(t.n + ins.n);

    memcpy(r.p, t.p, o);
    memcpy(r.p + o, ins.p, ins.n);
    memcpy(r.p + o + ins.n, t.p + o, t.n - o);

    G->push(r.p, r.n);

    free(t.p); free(off.p); free(ins.p); free(r.p);
    return 0;
}

static int txt_del(u8 *d, uint32_t n) {
    Buf ln = G->pop(), off = G->pop(), t = G->pop();
    DWORD o = rd32(off.p), l = rd32(ln.p);
    Buf r = mbuf(t.n - l);

    memcpy(r.p, t.p, o);
    memcpy(r.p + o, t.p + o + l, t.n - o - l);

    G->push(r.p, r.n);

    free(t.p); free(off.p); free(ln.p); free(r.p);
    return 0;
}

static int txt_rep(u8 *d, uint32_t n) {
    Buf rep = G->pop(), ln = G->pop(), off = G->pop(), t = G->pop();
    DWORD o = rd32(off.p), l = rd32(ln.p);
    Buf r = mbuf(t.n - l + rep.n);

    memcpy(r.p, t.p, o);
    memcpy(r.p + o, rep.p, rep.n);
    memcpy(r.p + o + rep.n, t.p + o + l, t.n - o - l);

    G->push(r.p, r.n);

    free(t.p); free(off.p); free(ln.p); free(rep.p); free(r.p);
    return 0;
}

static int txt_nl(u8 *d, uint32_t n) {
    u8 c = '\n';
    G->push(&c, 1);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:TXT:U8LEN", txt_u8len);
    h->op_name("CVM1:TXT:FIND", txt_find);
    h->op_name("CVM1:TXT:LINES", txt_lines);
    h->op_name("CVM1:TXT:LINE", txt_line);
    h->op_name("CVM1:TXT:INS", txt_ins);
    h->op_name("CVM1:TXT:DEL", txt_del);
    h->op_name("CVM1:TXT:REP", txt_rep);
    h->op_name("CVM1:TXT:NL", txt_nl);
}
