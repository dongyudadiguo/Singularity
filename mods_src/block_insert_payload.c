#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
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
static int zero32(const u8 *p){ for(int i=0;i<32;i++) if(p[i]) return 0; return 1; }
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 pn = cvm_payload_size();
    u32 po = 0;
    if (pn >= 72) {
        /* extended payload: target_key[32] + offset[u32] + token[32] + payload_size[u32] + payload */
        u8 h[32];
        cvm_resolve_payload_hash(p, h);
        po = 32;
    }
    if (pn < po + 40) { cont(); return; }
    u32 off = *(u32*)(p + po);
    if (off == 0xffffffffu) off = *(u32*)pop(4);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (len < 32 || off > len - 32) { cont(); return; }
    u32 ins_payload = *(u32*)(p + po + 36);
    if (pn < po + 40 + ins_payload) { cont(); return; }
    u32 add = 36 + ins_payload;
    if (len + add > MAX_BLOCK) { cont(); return; }
    memmove(base + off + add, base + off, len - off);
    memcpy(base + off, p + po + 4, 32);
    memcpy(base + off + 32, p + po + 36, 4 + ins_payload);
    cvm_cached_set_len(len + add);
    if (po) cvm_cache_flush();
    cont();
}
