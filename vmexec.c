#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

extern __declspec(dllimport) Fn imp;
extern __declspec(dllimport) Fn find(H h);
extern __declspec(dllimport) u8 *cvm_current_base(void);
extern __declspec(dllimport) u8 *cvm_current_key(void);
extern __declspec(dllimport) void cvm_set_current(const H k, u8 *base);
extern __declspec(dllimport) void cvm_advance(H next);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) void cvm_upload_async(const u8 *p, u32 n);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_hash_same(const H a, const H b);

static void start_fn(Fn f) {
    imp = f;
}

/*
 * cvm_exec(token) works as the VM dispatcher:
 *   1. Directly try mods/<token>.dll!run.  On hit, set imp and return.
 *   2. On miss, treat token as a block key.  Resolving the block checks the
 *      one-entry cache first; on cache hit it verifies hash/content consistency
 *      and schedules non-blocking user-override + file upload if dirty.  On
 *      cache miss it requests user override and falls back to getfirstchild.
 *   3. Enter the resolved block by recording current key/base, then continue
 *      the dispatch loop with the first token in that block.  cvm_advance()
 *      consumes token[32] + payload_size[u32] + payload bytes.
 */
__declspec(dllexport) void cvm_exec(const H in) {
    H token, h;
    Fn f;

    memcpy(token, in, 32);
    for (;;) {
        f = find(token);
        if (f) { start_fn(f); return; }

        cvm_resolve_payload_hash(token, h);
        cvm_set_current(token, cvm_cached_base());
        cvm_advance(token);
    }
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
    memcpy(k, cvm_current_key(), 32);
    cvm_exec(k);
}
