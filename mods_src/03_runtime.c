// 03_runtime — 执行、跳转、帧、变量
// gcc -shared mods_src/03_runtime.c -Os -s -o mods/03_runtime.dll

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

// Frame layout must match vm.c exactly (same compiler, same arch)
typedef struct {
    uint32_t off;
    Buf f;
    u8 key[H];
} Frame;

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

static uint32_t pop_u32(void) { Buf b = h->pop(); return b.n >= 4 ? U(b.p) : 0; }
static void push_u32(uint32_t v) { u8 buf[4]; WU(buf, v); h->push(buf, 4); }

// Helper: get current frame pointer via h->cur
static Frame *curf(void) { return (Frame *)h->cur; }

// RUN, ENTER, ADV: execution control
static void run_op(void) {
    if (h->plen >= H) h->run(h->pay);
    else h->next();
}

static void enter_op(void) {
    if (h->plen >= H) h->enter(h->pay);
    else h->next();
}

static void adv_op(void) {
    h->adv();
    h->next_noadv();
}

static void ov_set(void) {
    Buf file = h->pop();
    Buf key = h->pop();
    h->override(key.p, file.p, file.n);
    h->next();
}

static void ov_touch(void) {
    h->touch();
    h->next();
}

// FLOW: control flow — directly manipulate curf()->off
static void flow_jmp(void) {
    if (h->plen >= 4) {
        Frame *f = curf();
        if (f) f->off = U(h->pay);
    }
    h->next_noadv();
}

static void flow_jrel(void) {
    if (h->plen >= 4) {
        Frame *f = curf();
        if (f) f->off += (int32_t)U(h->pay);
    }
    h->next_noadv();
}

static void flow_jz(void) {
    Buf cond = h->pop();
    int is_zero = 1;
    for (uint32_t i = 0; i < cond.n; i++) {
        if (cond.p[i]) { is_zero = 0; break; }
    }
    if (is_zero && h->plen >= 4) {
        Frame *f = curf();
        if (f) f->off = U(h->pay);
        h->next_noadv();
    } else {
        h->next();
    }
}

static void flow_jnz(void) {
    Buf cond = h->pop();
    int is_zero = 1;
    for (uint32_t i = 0; i < cond.n; i++) {
        if (cond.p[i]) { is_zero = 0; break; }
    }
    if (!is_zero && h->plen >= 4) {
        Frame *f = curf();
        if (f) f->off = U(h->pay);
        h->next_noadv();
    } else {
        h->next();
    }
}

static void flow_next(void) {
    h->next();
}

static void flow_end(void) {
    Frame *f = curf();
    if (f) f->off = f->f.n;
    h->next_noadv();
}

// CUR: current frame info — read from curf()
static void cur_file(void) {
    Frame *f = curf();
    if (f && f->f.p) h->push(f->f.p, f->f.n);
    else h->push(0, 0);
    h->next();
}

static void cur_key(void) {
    Frame *f = curf();
    if (f) h->push(f->key, H);
    else { u8 z[H]; memset(z, 0, H); h->push(z, H); }
    h->next();
}

static void cur_off(void) {
    Frame *f = curf();
    push_u32(f ? f->off : 0);
    h->next();
}

static void cur_setoff(void) {
    if (h->plen >= 4) {
        Frame *f = curf();
        if (f) f->off = U(h->pay);
    }
    h->next();
}

// VAR: variables (512 slots, key is 32-byte token)
#define MAX_VARS 512
static struct { u8 key[H]; Buf val; } vars[MAX_VARS];
static int varn = 0;

static int var_find(u8 *key) {
    for (int i = 0; i < varn; i++) {
        if (!memcmp(vars[i].key, key, H)) return i;
    }
    return -1;
}

static void var_set(void) {
    Buf val = h->pop();
    Buf key = h->pop();
    
    int idx = var_find(key.p);
    if (idx >= 0) {
        free(vars[idx].val.p);
        vars[idx].val.p = malloc(val.n);
        if (vars[idx].val.p) {
            memcpy(vars[idx].val.p, val.p, val.n);
            vars[idx].val.n = val.n;
        }
    } else if (varn < MAX_VARS) {
        idx = varn++;
        memcpy(vars[idx].key, key.p, H);
        vars[idx].val.p = malloc(val.n);
        if (vars[idx].val.p) {
            memcpy(vars[idx].val.p, val.p, val.n);
            vars[idx].val.n = val.n;
        }
    }
    h->next();
}

static void var_get(void) {
    Buf key = h->pop();
    int idx = var_find(key.p);
    if (idx >= 0 && vars[idx].val.p) {
        h->push(vars[idx].val.p, vars[idx].val.n);
    } else {
        h->push(0, 0);
    }
    h->next();
}

static void var_has(void) {
    Buf key = h->pop();
    int idx = var_find(key.p);
    push_u32(idx >= 0 ? 1 : 0);
    h->next();
}

static void var_del(void) {
    Buf key = h->pop();
    int idx = var_find(key.p);
    if (idx >= 0) {
        free(vars[idx].val.p);
        vars[idx].val.p = 0;
        vars[idx].val.n = 0;
        for (int i = idx; i < varn - 1; i++) {
            vars[i] = vars[i + 1];
        }
        varn--;
    }
    h->next();
}

static void var_clear(void) {
    for (int i = 0; i < varn; i++) {
        free(vars[i].val.p);
        vars[i].val.p = 0;
        vars[i].val.n = 0;
    }
    varn = 0;
    h->next();
}

void cvm_init(Host *host) {
    h = host;
    h->op_name("RUN", run_op);
    h->op_name("ENTER", enter_op);
    h->op_name("ADV", adv_op);
    h->op_name("OV:SET", ov_set);
    h->op_name("OV:TOUCH", ov_touch);
    h->op_name("FLOW:JMP", flow_jmp);
    h->op_name("FLOW:JREL", flow_jrel);
    h->op_name("FLOW:JZ", flow_jz);
    h->op_name("FLOW:JNZ", flow_jnz);
    h->op_name("FLOW:NEXT", flow_next);
    h->op_name("FLOW:END", flow_end);
    h->op_name("CUR:FILE", cur_file);
    h->op_name("CUR:KEY", cur_key);
    h->op_name("CUR:OFF", cur_off);
    h->op_name("CUR:SETOFF", cur_setoff);
    h->op_name("VAR:SET", var_set);
    h->op_name("VAR:GET", var_get);
    h->op_name("VAR:HAS", var_has);
    h->op_name("VAR:DEL", var_del);
    h->op_name("VAR:CLEAR", var_clear);
}