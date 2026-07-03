#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void) {
    u32 off;
    u8 out[36];
    memset(out, 0, sizeof(out));
    if (cvm_payload_size() >= 4) off = *(u32*)cvm_payload();
    else off = *(u32*)pop(4);
    u8 *base = cvm_cached_base();
    u32 len = cvm_cached_len();
    if (off + 36 <= len) memcpy(out, base + off, 36);
    push(out, 36);
    cont();
}
