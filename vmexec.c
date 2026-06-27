#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

extern __declspec(dllimport) Fn imp;
extern __declspec(dllimport) Fn find(H h);
extern __declspec(dllimport) void cvm_firstchild(H p, H c);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_current_base(void);
extern __declspec(dllimport) u8 *cvm_current_key(void);
extern __declspec(dllimport) void cvm_set_current(const H k, u8 *base);
extern __declspec(dllimport) void cvm_cache_flush(void);
extern __declspec(dllimport) int cvm_hash_same(const H a, const H b);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_upload_async(const u8 *p, u32 n);

static void start_fn(Fn f, const H k, u8 *base) {
    cvm_set_current(k, base);
    imp = f;
}

__declspec(dllexport) void cvm_exec(const H k) {
    H h, child;
    Fn f;

    cvm_cache_flush();

    f = find((u8*)k);
    if (f) { start_fn(f, k, cvm_current_base()); return; }

    cvm_resolve_payload_hash(k, h);
    f = find(h);
    if (!f) { cvm_firstchild(h, child); f = find(child); }
    start_fn(f, k, cvm_cached_base());
}

__declspec(dllexport) void cvm_exec_payload(H k) {
    H oldh;
    u32 n = cvm_payload_size();
    u8 *p = cvm_payload();

    if (n >= 32) memcpy(k, p, 32);
    cvm_resolve_payload_hash(k, oldh);
    if (!cvm_hash_same(oldh, k) && n >= 32) {
        memcpy(p, oldh, 32);
        memcpy(k, oldh, 32);
        cvm_upload_async(cvm_current_base(), cvm_cached_len());
    }
    cvm_exec(k);
}

__declspec(dllexport) void cvm_reexec(void) {
    H k;
    Fn f;
    memcpy(k, cvm_current_key(), 32);
    f = find(k);
    if (f) start_fn(f, k, cvm_current_base());
    else cvm_set_current(k, cvm_current_base());
}
