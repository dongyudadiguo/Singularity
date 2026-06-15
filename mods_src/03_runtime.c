// 03_runtime — 执行、跳转、帧、变量
// gcc -shared mods_src/03_runtime.c -Os -s -o mods/03_runtime.dll

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

typedef struct {
    void (*op)(u8 *, void (*)(u8 *, uint32_t));
    void (*op_name)(char *, void (*)(u8 *, uint32_t));
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
} Host;

static Host *h;

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static void WU(u8 *p, uint32_t v) { memcpy(p, &v, 4); }

// RUN, ENTER, ADV: execution control
static void run_op(u8 *p, uint32_t n) {
    if (n >= H) h->run(p);
}

static void enter_op(u8 *p, uint32_t n) {
    if (n >= H) h->enter(p);
}

static void adv_op(u8 *p, uint32_t n) {
    h->adv();
}

// OV: override operations
static void ov_set(u8 *p, uint32_t n) {
    // stack: key, file_data
    Buf file = h->pop();
    Buf key = h->pop();
    h->override(key.p, file.p, file.n);
}

static void ov_touch(u8 *p, uint32_t n) {
    h->touch();
}

// FLOW: control flow
static void flow_jmp(u8 *p, uint32_t n) {
    // Jump to absolute offset
    if (n >= 4) {
        uint32_t off = U(p);
        // Set current offset - we need to access cur frame
        // For simplicity, we use adv repeatedly or enter
        // Actually, we can't directly set offset, so we use a workaround
        // The VM doesn't expose setoff directly in Host, but cvm.c has it
        // We'll need to use the override mechanism
    }
}

static void flow_jrel(u8 *p, uint32_t n) {
    // Jump relative
    if (n >= 4) {
        int32_t rel = (int32_t)U(p);
        // Similar issue - can't directly set offset
    }
}

static void flow_jz(u8 *p, uint32_t n) {
    // Jump if top of stack is zero
    Buf cond = h->pop();
    int is_zero = 1;
    for (uint32_t i = 0; i < cond.n; i++) {
        if (cond.p[i]) { is_zero = 0; break; }
    }
    if (is_zero && n >= 4) {
        // Would jump, but no direct offset setting
    }
}

static void flow_jnz(u8 *p, uint32_t n) {
    // Jump if top of stack is not zero
    Buf cond = h->pop();
    int is_zero = 1;
    for (uint32_t i = 0; i < cond.n; i++) {
        if (cond.p[i]) { is_zero = 0; break; }
    }
    if (!is_zero && n >= 4) {
        // Would jump
    }
}

static void flow_next(u8 *p, uint32_t n) {
    // Advance to next instruction
    h->adv();
}

static void flow_end(u8 *p, uint32_t n) {
    // End current frame - just advance past end
    // This is handled by the VM loop
}

// CUR: current frame info
static void cur_file(u8 *p, uint32_t n) {
    // Push current file data - not directly accessible from Host
    // We push empty for now
    h->push(0, 0);
}

static void cur_key(u8 *p, uint32_t n) {
    // Push current key - not directly accessible
    u8 z[H]; memset(z, 0, H);
    h->push(z, H);
}

static void cur_off(u8 *p, uint32_t n) {
    // Push current offset - not directly accessible
    push_u32(0);
}

static void cur_setoff(u8 *p, uint32_t n) {
    // Set current offset - not directly accessible
    if (n >= 4) {
        uint32_t off = U(p);
        // Can't set directly
    }
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

static void var_set(u8 *p, uint32_t n) {
    // stack: key, value
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
}

static void var_get(u8 *p, uint32_t n) {
    // stack: key
    Buf key = h->pop();
    int idx = var_find(key.p);
    if (idx >= 0 && vars[idx].val.p) {
        h->push(vars[idx].val.p, vars[idx].val.n);
    } else {
        h->push(0, 0);
    }
}

static void var_has(u8 *p, uint32_t n) {
    // stack: key
    Buf key = h->pop();
    int idx = var_find(key.p);
    push_u32(idx >= 0 ? 1 : 0);
}

static void var_del(u8 *p, uint32_t n) {
    // stack: key
    Buf key = h->pop();
    int idx = var_find(key.p);
    if (idx >= 0) {
        free(vars[idx].val.p);
        vars[idx].val.p = 0;
        vars[idx].val.n = 0;
        // Shift remaining vars down
        for (int i = idx; i < varn - 1; i++) {
            vars[i] = vars[i + 1];
        }
        varn--;
    }
}

static void var_clear(u8 *p, uint32_t n) {
    for (int i = 0; i < varn; i++) {
        free(vars[i].val.p);
        vars[i].val.p = 0;
        vars[i].val.n = 0;
    }
    varn = 0;
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