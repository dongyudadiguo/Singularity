#include "../cvm_host.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VN 512

static Host *H;

typedef struct {
    int used;
    u8 key[32];
    Buf val;
} Var;

static Var vars[VN];

static uint32_t u32(u8 *p) {
    uint32_t x = 0;
    memcpy(&x, p, 4);
    return x;
}

static void put32(u8 *p, uint32_t x) {
    memcpy(p, &x, 4);
}

static void take32(Buf *b, u8 out[32]) {
    memset(out, 0, 32);
    if (b->n)
        memcpy(out, b->p, b->n > 32 ? 32 : b->n);
}

static Buf clone(u8 *p, DWORD n) {
    Buf b = { malloc(n), n };
    if (n) memcpy(b.p, p, n);
    return b;
}

static uint32_t pop32() {
    Buf b = H->pop();
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static void push32(uint32_t x) {
    u8 b[4];
    put32(b, x);
    H->push(b, 4);
}

static Var *find(u8 key[32]) {
    for (int i = 0; i < VN; i++)
        if (vars[i].used && !memcmp(vars[i].key, key, 32))
            return vars + i;
    return 0;
}

static Var *slot(u8 key[32]) {
    Var *v = find(key);

    if (v)
        return v;

    for (int i = 0; i < VN; i++) {
        if (!vars[i].used) {
            vars[i].used = 1;
            memcpy(vars[i].key, key, 32);
            return vars + i;
        }
    }

    return vars;
}

static void op_run(u8 *d, uint32_t n) {
    Buf b = H->pop();
    u8 k[32];

    take32(&b, k);
    free(b.p);

    H->run(k);
}

static void op_enter(u8 *d, uint32_t n) {
    Buf b = H->pop();
    u8 k[32];

    take32(&b, k);
    free(b.p);

    H->enter(k);
}

static void op_adv(u8 *d, uint32_t n) {
    H->adv();
}

static void op_ov_set(u8 *d, uint32_t n) {
    Buf file = H->pop();
    Buf key = H->pop();
    u8 k[32];

    take32(&key, k);
    H->override(k, file.p, file.n);

    free(key.p);
    free(file.p);

    H->adv();
}

static void op_ov_touch(u8 *d, uint32_t n) {
    H->touch();
    H->adv();
}

static void op_jmp(u8 *d, uint32_t n) {
    if (n >= 4)
        H->cur->off = u32(d);
}

static void op_jrel(u8 *d, uint32_t n) {
    if (n >= 4) {
        int32_t x = 0;
        memcpy(&x, d, 4);
        H->cur->off += x;
    }
}

static void op_jz(u8 *d, uint32_t n) {
    uint32_t c = pop32();

    if (!c && n >= 4)
        H->cur->off = u32(d);
    else
        H->adv();
}

static void op_jnz(u8 *d, uint32_t n) {
    uint32_t c = pop32();

    if (c && n >= 4)
        H->cur->off = u32(d);
    else
        H->adv();
}

static void op_next(u8 *d, uint32_t n) {
    H->adv();
}

static void op_end(u8 *d, uint32_t n) {
    H->cur->off = H->cur->f.n;
}

static void op_cur_file(u8 *d, uint32_t n) {
    H->push(H->cur->f.p, H->cur->f.n);
    H->adv();
}

static void op_cur_key(u8 *d, uint32_t n) {
    H->push(H->cur->key, 32);
    H->adv();
}

static void op_cur_off(u8 *d, uint32_t n) {
    push32(H->cur->off);
    H->adv();
}

static void op_cur_setoff(u8 *d, uint32_t n) {
    H->cur->off = pop32();
}

static void op_var_set(u8 *d, uint32_t n) {
    Buf val = H->pop();
    Buf key = H->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = slot(k);

    free(v->val.p);
    v->val = clone(val.p, val.n);

    free(key.p);
    free(val.p);

    H->adv();
}

static void op_var_get(u8 *d, uint32_t n) {
    Buf key = H->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = find(k);

    if (v)
        H->push(v->val.p, v->val.n);
    else
        H->push(0, 0);

    free(key.p);
    H->adv();
}

static void op_var_has(u8 *d, uint32_t n) {
    Buf key = H->pop();
    u8 k[32];

    take32(&key, k);
    push32(find(k) ? 1 : 0);

    free(key.p);
    H->adv();
}

static void op_var_del(u8 *d, uint32_t n) {
    Buf key = H->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = find(k);
    if (v) {
        free(v->val.p);
        memset(v, 0, sizeof *v);
    }

    free(key.p);
    H->adv();
}

static void op_var_clear(u8 *d, uint32_t n) {
    for (int i = 0; i < VN; i++) {
        if (vars[i].used) {
            free(vars[i].val.p);
            memset(vars + i, 0, sizeof vars[i]);
        }
    }

    H->adv();
}

__declspec(dllexport)
void cvm_init(Host *h) {
    H = h;

    h->op_name("RUN", op_run);
    h->op_name("ENTER", op_enter);
    h->op_name("ADV", op_adv);

    h->op_name("OV:SET", op_ov_set);
    h->op_name("OV:TOUCH", op_ov_touch);

    h->op_name("FLOW:JMP", op_jmp);
    h->op_name("FLOW:JREL", op_jrel);
    h->op_name("FLOW:JZ", op_jz);
    h->op_name("FLOW:JNZ", op_jnz);
    h->op_name("FLOW:NEXT", op_next);
    h->op_name("FLOW:END", op_end);

    h->op_name("CUR:FILE", op_cur_file);
    h->op_name("CUR:KEY", op_cur_key);
    h->op_name("CUR:OFF", op_cur_off);
    h->op_name("CUR:SETOFF", op_cur_setoff);

    h->op_name("VAR:SET", op_var_set);
    h->op_name("VAR:GET", op_var_get);
    h->op_name("VAR:HAS", op_var_has);
    h->op_name("VAR:DEL", op_var_del);
    h->op_name("VAR:CLEAR", op_var_clear);
}