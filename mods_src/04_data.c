#include "../cvm_host.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Host *H;

enum {
    K_ESC = 27,
    K_ENTER = 13,
    K_BACK = 8,
    K_DEL = 127,
    K_TAB = 9,
    K_SPACE = 32,

    K_LEFT = 1000,
    K_RIGHT,
    K_UP,
    K_DOWN,
    K_HOME,
    K_END,
    K_PGUP,
    K_PGDN
};

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
    H->push(b, 4);
}

static uint32_t pop32() {
    Buf b = H->pop();
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static uint32_t lcnt(Buf *b) {
    return b->n >= 4 ? u32(b->p) : 0;
}

static int elem(Buf *b, uint32_t idx, DWORD *po, DWORD *pn) {
    uint32_t c = lcnt(b);
    DWORD o = 4;

    for (uint32_t i = 0; i < c; i++) {
        if (o + 4 > b->n)
            return 0;

        uint32_t n = u32(b->p + o);
        o += 4;

        if (o + n > b->n)
            return 0;

        if (i == idx) {
            *po = o;
            *pn = n;
            return 1;
        }

        o += n;
    }

    return 0;
}

static void add(u8 **p, DWORD *n, u8 *x, DWORD xn) {
    *p = realloc(*p, *n + 4 + xn);
    put32(*p + *n, xn);
    *n += 4;

    if (xn)
        memcpy(*p + *n, x, xn);

    *n += xn;
}

static void op_lst_new(u8 *d, uint32_t n) {
    u8 b[4] = {0};
    H->push(b, 4);
    H->adv();
}

static void op_lst_count(u8 *d, uint32_t n) {
    Buf b = H->pop();
    push32(lcnt(&b));
    free(b.p);
    H->adv();
}

static void op_lst_get(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf b = H->pop();
    DWORD o, sz;

    if (elem(&b, idx, &o, &sz))
        H->push(b.p + o, sz);
    else
        H->push(0, 0);

    free(b.p);
    H->adv();
}

static void op_lst_push(u8 *d, uint32_t n) {
    Buf e = H->pop();
    Buf b = H->pop();

    uint32_t c = lcnt(&b);
    DWORD tail = b.n >= 4 ? b.n - 4 : 0;
    DWORD on = 4 + tail + 4 + e.n;
    u8 *o = malloc(on);

    put32(o, c + 1);

    if (tail)
        memcpy(o + 4, b.p + 4, tail);

    put32(o + 4 + tail, e.n);

    if (e.n)
        memcpy(o + 8 + tail, e.p, e.n);

    H->push(o, on);

    free(o);
    free(b.p);
    free(e.p);

    H->adv();
}

static void op_lst_del(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf b = H->pop();

    uint32_t c = lcnt(&b);

    if (idx >= c) {
        H->push(b.p, b.n);
        free(b.p);
        H->adv();
        return;
    }

    u8 *o = malloc(4);
    DWORD on = 4;
    uint32_t outc = 0;

    put32(o, 0);

    for (uint32_t i = 0; i < c; i++) {
        DWORD p, sz;

        if (i != idx && elem(&b, i, &p, &sz)) {
            add(&o, &on, b.p + p, sz);
            outc++;
        }
    }

    put32(o, outc);

    H->push(o, on);

    free(o);
    free(b.p);

    H->adv();
}

static void op_lst_join(u8 *d, uint32_t n) {
    Buf sep = H->pop();
    Buf b = H->pop();

    uint32_t c = lcnt(&b);
    u8 *o = 0;
    DWORD on = 0;

    for (uint32_t i = 0; i < c; i++) {
        DWORD p, sz;

        if (!elem(&b, i, &p, &sz))
            continue;

        if (i && sep.n) {
            o = realloc(o, on + sep.n);
            memcpy(o + on, sep.p, sep.n);
            on += sep.n;
        }

        if (sz) {
            o = realloc(o, on + sz);
            memcpy(o + on, b.p + p, sz);
            on += sz;
        }
    }

    H->push(o, on);

    free(o);
    free(b.p);
    free(sep.p);

    H->adv();
}

static void key_const(uint32_t x) {
    push32(x);
    H->adv();
}

static void op_key_esc(u8 *d, uint32_t n) { key_const(K_ESC); }
static void op_key_enter(u8 *d, uint32_t n) { key_const(K_ENTER); }
static void op_key_back(u8 *d, uint32_t n) { key_const(K_BACK); }
static void op_key_del(u8 *d, uint32_t n) { key_const(K_DEL); }
static void op_key_tab(u8 *d, uint32_t n) { key_const(K_TAB); }
static void op_key_space(u8 *d, uint32_t n) { key_const(K_SPACE); }
static void op_key_left(u8 *d, uint32_t n) { key_const(K_LEFT); }
static void op_key_right(u8 *d, uint32_t n) { key_const(K_RIGHT); }
static void op_key_up(u8 *d, uint32_t n) { key_const(K_UP); }
static void op_key_down(u8 *d, uint32_t n) { key_const(K_DOWN); }
static void op_key_home(u8 *d, uint32_t n) { key_const(K_HOME); }
static void op_key_end(u8 *d, uint32_t n) { key_const(K_END); }
static void op_key_pgup(u8 *d, uint32_t n) { key_const(K_PGUP); }
static void op_key_pgdn(u8 *d, uint32_t n) { key_const(K_PGDN); }

static void op_key_code(u8 *d, uint32_t n) {
    Buf e = H->pop();
    push32(e.n >= 4 ? u32(e.p) : 0);
    free(e.p);
    H->adv();
}

static void op_key_mods(u8 *d, uint32_t n) {
    Buf e = H->pop();
    push32(e.n >= 8 ? u32(e.p + 4) : 0);
    free(e.p);
    H->adv();
}

static void op_key_ascii(u8 *d, uint32_t n) {
    Buf e = H->pop();
    push32(e.n >= 12 ? u32(e.p + 8) : 0);
    free(e.p);
    H->adv();
}

__declspec(dllexport)
void cvm_init(Host *h) {
    H = h;

    h->op_name("LST:NEW", op_lst_new);
    h->op_name("LST:COUNT", op_lst_count);
    h->op_name("LST:GET", op_lst_get);
    h->op_name("LST:PUSH", op_lst_push);
    h->op_name("LST:DEL", op_lst_del);
    h->op_name("LST:JOIN", op_lst_join);

    h->op_name("KEY:ESC", op_key_esc);
    h->op_name("KEY:ENTER", op_key_enter);
    h->op_name("KEY:BACK", op_key_back);
    h->op_name("KEY:DEL", op_key_del);
    h->op_name("KEY:TAB", op_key_tab);
    h->op_name("KEY:SPACE", op_key_space);
    h->op_name("KEY:LEFT", op_key_left);
    h->op_name("KEY:RIGHT", op_key_right);
    h->op_name("KEY:UP", op_key_up);
    h->op_name("KEY:DOWN", op_key_down);
    h->op_name("KEY:HOME", op_key_home);
    h->op_name("KEY:END", op_key_end);
    h->op_name("KEY:PGUP", op_key_pgup);
    h->op_name("KEY:PGDN", op_key_pgdn);
    h->op_name("KEY:CODE", op_key_code);
    h->op_name("KEY:ASCII", op_key_ascii);
    h->op_name("KEY:MODS", op_key_mods);
}