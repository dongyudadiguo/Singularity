#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const u8 *k, u8 *h);
extern __declspec(dllimport) void cvm_cache_flush(void);
#define MAX_BLOCK (1u << 20)
/*
 * payload forms:
 *   off[u32] + token[32] + payload_size[u32] + payload
 *   target_key[32] + off[u32] + token[32] + payload_size[u32] + payload  (extended)
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 pn = cvm_payload_size();
    u32 po = 0;
    if (pn >= 8 + 32 + 4) {
        /* detect extended: first 32 look like key and next u32 is offset */
        /* Prefer extended when pn is large enough for key+off+token+plen */
        if (pn >= 32 + 4 + 32 + 4) {
            u8 h[32];
            cvm_resolve_payload_hash(p, h);
            po = 32;
        }
    }
    if (pn < po + 4 + 32 + 4) { cont(); return; }
    u32 off = *(u32*)(p + po);
    if (off == 0xffffffffu) off = *(u32*)pop(4);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (off > len) { cont(); return; }
    u8 *tok = p + po + 4;
    u32 ins_payload = *(u32*)(p + po + 4 + 32);
    if (pn < po + 4 + 32 + 4 + ins_payload) { cont(); return; }
    u32 add = 8 + 32 + ins_payload;
    if (len + add > MAX_BLOCK) { cont(); return; }
    memmove(base + off + add, base + off, len - off);
    bl_write(base + off, tok, 32, p + po + 4 + 32 + 4, ins_payload);
    cvm_cached_set_len(len + add);
    if (po) cvm_cache_flush();
    cont();
}
