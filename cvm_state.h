#ifndef CVM_STATE_H
#define CVM_STATE_H

#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <setjmp.h>
#pragma comment(lib, "advapi32.lib")

#ifndef CVM_TYPES_DEFINED
#define CVM_TYPES_DEFINED
typedef unsigned char u8;
typedef unsigned u32;
typedef unsigned long long u64;
typedef u8 H[32];
#endif

#define CVM_STACK_CAP 256
#define CVM_SCOPE_CAP 64
#define CVM_VAR_CAP 64
#define CVM_VIEW_CAP 64

typedef struct { H id; H val; } CvmVar;
typedef struct { CvmVar vars[CVM_VAR_CAP]; u32 count; } CvmScope;

typedef struct {
    H stack[CVM_STACK_CAP];
    u32 sp;
    CvmScope scopes[CVM_SCOPE_CAP];
    u32 depth;
    u8 *payload;
    u32 payload_len;
    H cur_hash;
    u8 *chain;
    u32 chain_len;
    u32 off;
    u32 span;
    jmp_buf *ret_jb;
    u32 next_off;
    H view_hash;
    u64 view_index;
    H view_hash_stack[CVM_VIEW_CAP];
    u64 view_index_stack[CVM_VIEW_CAP];
    u32 view_sp;
    HWND surface_hwnd;
    u64 surface_event;
    u64 surface_x;
    u64 surface_y;
    u64 surface_w;
    u64 surface_h;
    u32 chain_start;
} CvmState;

static CvmState* cvm_state(void) {
    static CvmState *s;
    static HANDLE map;
    if (s) return s;
    map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, "Local\\CVM_State");
    if (!map) map = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(CvmState), "Local\\CVM_State");
    if (!map) return 0;
    s = (CvmState*)MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CvmState));
    return s;
}

static int cvm_sha256(const u8 *data, u32 len, H out) {
    HCRYPTPROV hp = 0;
    HCRYPTHASH hh = 0;
    DWORD sz = 32;
    int ok = 0;
    if (CryptAcquireContextW(&hp, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hp, CALG_SHA_256, 0, 0, &hh)) {
            if (CryptHashData(hh, data, len, 0)) ok = !!CryptGetHashParam(hh, HP_HASHVAL, out, &sz, 0);
            CryptDestroyHash(hh);
        }
        CryptReleaseContext(hp, 0);
    }
    return ok;
}

static void cvm_zero(H h) { memset(h, 0, 32); }

static void cvm_u64_to_h(u64 v, H out) {
    memset(out, 0, 32);
    for (int i = 0; i < 8; i++) out[i] = (u8)(v >> (i * 8));
}

static u64 cvm_h_to_u64(const H h) {
    u64 v = 0;
    for (int i = 0; i < 8; i++) v |= ((u64)h[i]) << (i * 8);
    return v;
}

static int cvm_truth(const H h) {
    for (int i = 0; i < 32; i++) if (h[i]) return 1;
    return 0;
}

static int cvm_push(const H v) {
    CvmState *s = cvm_state();
    if (!s || s->sp >= CVM_STACK_CAP) return 0;
    memcpy(s->stack[s->sp++], v, 32);
    return 1;
}

static int cvm_pop(H out) {
    CvmState *s = cvm_state();
    if (!s || !s->sp) { cvm_zero(out); return 0; }
    memcpy(out, s->stack[--s->sp], 32);
    return 1;
}

static CvmScope* cvm_cur_scope(void) {
    CvmState *s = cvm_state();
    if (!s || s->depth >= CVM_SCOPE_CAP) return 0;
    return &s->scopes[s->depth];
}

static int cvm_var_set(const H id, const H val) {
    CvmScope *sc = cvm_cur_scope();
    if (!sc) return 0;
    for (u32 i = 0; i < sc->count; i++)
        if (memcmp(sc->vars[i].id, id, 32) == 0) { memcpy(sc->vars[i].val, val, 32); return 1; }
    if (sc->count >= CVM_VAR_CAP) return 0;
    memcpy(sc->vars[sc->count].id, id, 32);
    memcpy(sc->vars[sc->count].val, val, 32);
    sc->count++;
    return 1;
}

static int cvm_var_read(const H id, H out) {
    CvmState *s = cvm_state();
    if (!s) { cvm_zero(out); return 0; }
    for (int d = (int)s->depth; d >= 0; d--)
        for (u32 i = 0; i < s->scopes[d].count; i++)
            if (memcmp(s->scopes[d].vars[i].id, id, 32) == 0) { memcpy(out, s->scopes[d].vars[i].val, 32); return 1; }
    cvm_zero(out);
    return 0;
}

static int cvm_var_write(const H id, const H val) {
    CvmState *s = cvm_state();
    if (!s) return 0;
    for (int d = (int)s->depth; d >= 0; d--)
        for (u32 i = 0; i < s->scopes[d].count; i++)
            if (memcmp(s->scopes[d].vars[i].id, id, 32) == 0) { memcpy(s->scopes[d].vars[i].val, val, 32); return 1; }
    return cvm_var_set(id, val);
}

static int cvm_scope_start(void) {
    CvmState *s = cvm_state();
    if (!s || s->depth + 1 >= CVM_SCOPE_CAP) return 0;
    s->depth++;
    s->scopes[s->depth].count = 0;
    return 1;
}

static int cvm_scope_end(void) {
    CvmState *s = cvm_state();
    if (!s || !s->depth) return 0;
    s->scopes[s->depth].count = 0;
    s->depth--;
    return 1;
}

#endif
