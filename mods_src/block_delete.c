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
__declspec(dllexport) void run(void) {
    u32 off;
    if (cvm_payload_size() >= 4) off = *(u32*)cvm_payload();
    else off = *(u32*)pop(4);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (len < 32 || off > len - 32) { cont(); return; }
    u32 n = *(u32*)(base + off + 32);
    u32 del = 36 + n;
    if (off + del > len - 32) { cont(); return; }
    memmove(base + off, base + off + del, len - off - del);
    cvm_cached_set_len(len - del);
    cont();
}
