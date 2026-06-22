#ifndef CONTINUE_H
#define CONTINUE_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "cvm_state.h"

static const H CEND = {0};

static u32 cspan_at(u8 *p) {
    return (u32)p[32] | ((u32)p[33] << 8) | ((u32)p[34] << 16) | ((u32)p[35] << 24);
}

static void* cfind(u8 *tok) {
    char path[74] = "mods/";
    for (int i = 0; i < 32; i++) sprintf(path + 5 + i * 2, "%02x", tok[i]);
    strcat(path, ".dll");
    HMODULE m = LoadLibraryA(path);
    return m ? (void*)GetProcAddress(m, "run") : 0;
}

static void ccont(u32 off) {
    CvmState *s = cvm_state();
    if (!s || !s->chain || off + 32 > s->chain_len) return;

    u8 *p = s->chain + off;
    if (memcmp(p, CEND, 32) == 0) return;
    if (off + 36 > s->chain_len) return;

    u32 sp = cspan_at(p);
    if (sp < 4 || off + 32 + sp > s->chain_len) return;

    s->off = off;
    s->span = sp;
    s->payload = p + 36;
    s->payload_len = sp - 4;

    void *fn = cfind(p);
    if (fn) ((void(*)())fn)();
}

static void cnext(void) {
    CvmState *s = cvm_state();
    if (!s) return;
    ccont(s->off + 32 + s->span);
}

static void cbegin(u8 *chain, u32 len) {
    CvmState *s = cvm_state();
    if (!s) return;
    s->chain = chain;
    s->chain_len = len;
    s->chain_start = 0;
    s->off = 0;
    s->span = 0;
    s->payload = 0;
    s->payload_len = 0;
    ccont(0);
}

typedef struct {
    u8 *payload;
    u32 payload_len;
    u8 *chain;
    u32 chain_len;
    u32 chain_start;
    u32 off;
    u32 span;
    jmp_buf *ret_jb;
    H cur_hash;
} CvmCallFrame;

static void cframe_save(CvmState *s, CvmCallFrame *f) {
    if (!s || !f) return;
    f->payload = s->payload;
    f->payload_len = s->payload_len;
    f->chain = s->chain;
    f->chain_len = s->chain_len;
    f->chain_start = s->chain_start;
    f->off = s->off;
    f->span = s->span;
    f->ret_jb = s->ret_jb;
    memcpy(f->cur_hash, s->cur_hash, 32);
}

static void cframe_restore(CvmState *s, CvmCallFrame *f) {
    if (!s || !f) return;
    s->payload = f->payload;
    s->payload_len = f->payload_len;
    s->chain = f->chain;
    s->chain_len = f->chain_len;
    s->chain_start = f->chain_start;
    s->off = f->off;
    s->span = f->span;
    s->ret_jb = f->ret_jb;
    memcpy(s->cur_hash, f->cur_hash, 32);
}

#endif
