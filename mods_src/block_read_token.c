#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
/* push token[32] + plen[u32] from offset (legacy 36-byte-ish) */
__declspec(dllexport) void run(void) {
    u32 off;
    if (cvm_payload_size() >= 4) off = *(u32*)cvm_payload();
    else off = *(u32*)pop(4);
    u8 out[36]; memset(out, 0, 36);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (bl_ok(base, len, off) && !bl_is_end(base + off) && bl_tlen(base + off) == 32) {
        memcpy(out, bl_token_c(base + off), 32);
        *(u32*)(out + 32) = bl_plen(base + off);
    }
    push(out, 36);
    cont();
}
