#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

extern __declspec(dllimport) Fn imp;
extern __declspec(dllimport) Fn find(H h);
extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cvm_restart_current(void);
extern __declspec(dllimport) void cvm_set_current(const H k, u8 *base);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);
extern __declspec(dllimport) int cvm_ret(void);

static void start_fn(Fn f) { imp = f; }

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

/* Advance ptr past current instruction. Returns 0 if stream ended (and ret failed). */
static int advance_ptr(void) {
    u32 tlen, plen;
    if (!ptr) return 0;
    tlen = *(u32 *)ptr;
    if (tlen > (1u << 20)) return 0;
    plen = *(u32 *)(ptr + 4 + tlen);
    if (plen > (1u << 20)) return 0;
    ptr += 8 + tlen + plen;
    for (;;) {
        tlen = *(u32 *)ptr;
        if (tlen != 0) return 1;
        if (!cvm_ret()) return 0;
        /* after ret, ptr is restored to caller instruction; skip it like cont() */
        tlen = *(u32 *)ptr;
        if (tlen > (1u << 20)) return 0;
        plen = *(u32 *)(ptr + 4 + tlen);
        if (plen > (1u << 20)) return 0;
        ptr += 8 + tlen + plen;
    }
}

__declspec(dllexport) void cvm_exec_instr(void) {
    for (;;) {
        u32 tlen;
        u8 *tok;
        H token, h;
        Fn f;
        if (!ptr) return;
        tlen = *(u32 *)ptr;
        if (tlen == 0) {
            if (!advance_ptr()) return;
            continue;
        }
        tok = ptr + 4;

        if (tlen == 32) {
            memcpy(token, tok, 32);
            f = find(token);
            if (f) { start_fn(f); return; }
            cvm_resolve_payload_hash(token, h);
            cvm_set_current(token, cvm_cached_base());
            continue;
        }

        if (tlen > 0 && tlen < (1u << 20)) {
            if (cvm_sha256(tok, tlen, token)) {
                f = find(token);
                if (f) { start_fn(f); return; }
                cvm_resolve_payload_hash(token, h);
                cvm_set_current(token, cvm_cached_base());
                continue;
            }
        }
        if (!advance_ptr()) return;
    }
}

__declspec(dllexport) void cvm_exec(const H in) {
    H token, h;
    Fn f;
    memcpy(token, in, 32);
    if (zero32(token)) return;
    f = find(token);
    if (f) { start_fn(f); return; }
    cvm_resolve_payload_hash(token, h);
    cvm_set_current(token, cvm_cached_base());
    cvm_exec_instr();
}

__declspec(dllexport) void cvm_exec_payload(H k) {
    u32 n = cvm_payload_size();
    u8 *p = cvm_payload();
    if (n >= 32) memcpy(k, p, 32);
    cvm_exec(k);
}

__declspec(dllexport) void cvm_reexec(void) {
    cvm_restart_current();
    cvm_exec_instr();
}
