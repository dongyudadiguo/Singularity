#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);
#define MAX_BLOCK (1u << 20)
/* stack: off[u32]; new payload = cvm_payload() */
__declspec(dllexport) void run(void) {
    u32 off = *(u32*)from(4);
    u8 *np = cvm_payload();
    u32 nn = cvm_payload_size();
    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (!bl_ok(b, l, off) || bl_is_end(b + off)) { cont(); return; }
    u32 tlen = bl_tlen(b + off);
    u32 old_sz = bl_instr_size(b + off);
    u32 new_sz = 8 + tlen + nn;
    if (l - old_sz + new_sz > MAX_BLOCK) { cont(); return; }
    memmove(b + off + new_sz, b + off + old_sz, l - (off + old_sz));
    *(u32 *)(b + off + 4 + tlen) = nn;
    if (nn) memcpy(b + off + 8 + tlen, np, nn);
    cvm_cached_set_len(l - old_sz + new_sz);
    cont();
}
