// 03_runtime: execution, flow, frame, variable operations

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define H 32
#define VN 512

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
    Buf b = h->pop();
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static void push32(uint32_t x) {
    u8 b[4];
    put32(b, x);
    h->push(b, 4);
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
    Buf b = h->pop();
    u8 k[32];

    take32(&b, k);
    free(b.p);

    h->run(k);
}

static void op_enter(u8 *d, uint32_t n) {
    Buf b = h->pop();
    u8 k[32];

    take32(&b, k);
    free(b.p);

    h->enter(k);
}

static void op_adv(u8 *d, uint32_t n) {
    h->adv();
}

static void op_ov_set(u8 *d, uint32_t n) {
    Buf file = h->pop();
    Buf key = h->pop();
    u8 k[32];

    take32(&key, k);
    h->override(k, file.p, file.n);

    free(key.p);
    free(file.p);

    h->adv();
}

static void op_ov_touch(u8 *d, uint32_t n) {
    h->touch();
    h->adv();
}

static void op_jmp(u8 *d, uint32_t n) {
    if (n >= 4)
        h->cur->off = u32(d);
}

static void op_jrel(u8 *d, uint32_t n) {
    if (n >= 4) {
        int32_t x = 0;
        memcpy(&x, d, 4);
        h->cur->off += x;
    }
}

static void op_jz(u8 *d, uint32_t n) {
    uint32_t c = pop32();

    if (!c && n >= 4)
        h->cur->off = u32(d);
    else
        h->adv();
}

static void op_jnz(u8 *d, uint32_t n) {
    uint32_t c = pop32();

    if (c && n >= 4)
        h->cur->off = u32(d);
    else
        h->adv();
}

static void op_next(u8 *d, uint32_t n) {
    h->adv();
}

static void op_end(u8 *d, uint32_t n) {
    h->cur->off = h->cur->f.n;
}

static void op_cur_file(u8 *d, uint32_t n) {
    h->push(h->cur->f.p, h->cur->f.n);
    h->adv();
}

static void op_cur_key(u8 *d, uint32_t n) {
    h->push(h->cur->key, 32);
    h->adv();
}

static void op_cur_off(u8 *d, uint32_t n) {
    push32(h->cur->off);
    h->adv();
}

static void op_cur_setoff(u8 *d, uint32_t n) {
    h->cur->off = pop32();
}

static void op_var_set(u8 *d, uint32_t n) {
    Buf val = h->pop();
    Buf key = h->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = slot(k);

    free(v->val.p);
    v->val = clone(val.p, val.n);

    free(key.p);
    free(val.p);

    h->adv();
}

static void op_var_get(u8 *d, uint32_t n) {
    Buf key = h->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = find(k);

    if (v)
        h->push(v->val.p, v->val.n);
    else
        h->push(0, 0);

    free(key.p);
    h->adv();
}

static void op_var_has(u8 *d, uint32_t n) {
    Buf key = h->pop();
    u8 k[32];

    take32(&key, k);
    push32(find(k) ? 1 : 0);

    free(key.p);
    h->adv();
}

static void op_var_del(u8 *d, uint32_t n) {
    Buf key = h->pop();
    u8 k[32];

    take32(&key, k);

    Var *v = find(k);
    if (v) {
        free(v->val.p);
        memset(v, 0, sizeof *v);
    }

    free(key.p);
    h->adv();
}

static void op_var_clear(u8 *d, uint32_t n) {
    for (int i = 0; i < VN; i++) {
        if (vars[i].used) {
            free(vars[i].val.p);
            memset(vars + i, 0, sizeof vars[i]);
        }
    }

    h->adv();
}

__declspec(dllexport)
void cvm_init(Host *host) {
    h = host;

    host->op_name("RUN", op_run);
    host->op_name("ENTER", op_enter);
    host->op_name("ADV", op_adv);

    host->op_name("OV:SET", op_ov_set);
    host->op_name("OV:TOUCH", op_ov_touch);

    host->op_name("FLOW:JMP", op_jmp);
    host->op_name("FLOW:JREL", op_jrel);
    host->op_name("FLOW:JZ", op_jz);
    host->op_name("FLOW:JNZ", op_jnz);
    host->op_name("FLOW:NEXT", op_next);
    host->op_name("FLOW:END", op_end);

    host->op_name("CUR:FILE", op_cur_file);
    host->op_name("CUR:KEY", op_cur_key);
    host->op_name("CUR:OFF", op_cur_off);
    host->op_name("CUR:SETOFF", op_cur_setoff);

    host->op_name("VAR:SET", op_var_set);
    host->op_name("VAR:GET", op_var_get);
    host->op_name("VAR:HAS", op_var_has);
    host->op_name("VAR:DEL", op_var_del);
    host->op_name("VAR:CLEAR", op_var_clear);
}