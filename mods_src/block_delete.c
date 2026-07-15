#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);
__declspec(dllexport) void run(void) {
    u32 off;
    if (cvm_payload_size() >= 4) off = *(u32*)cvm_payload();
    else off = *(u32*)pop(4);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (!bl_ok(base, len, off) || bl_is_end(base + off)) { cont(); return; }
    u32 del = bl_instr_size(base + off);
    if (off + del > len) { cont(); return; }
    memmove(base + off, base + off + del, len - off - del);
    cvm_cached_set_len(len - del);
    cont();
}
