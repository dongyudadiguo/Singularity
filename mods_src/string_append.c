#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
/* payload: dst_n[u32] + src_n[u32]
 * stack top-first: src[src_n], dst[dst_n]
 * NUL-terminated append into dst capacity; push dst[dst_n]
 */
__declspec(dllexport) void run(void){
    u8 *p = cvm_payload();
    u32 pn = cvm_payload_size();
    u32 dst_n = (pn >= 4) ? *(u32*)p : 256;
    u32 src_n = (pn >= 8) ? *(u32*)(p + 4) : 256;
    if (!dst_n) dst_n = 1;
    if (!src_n) src_n = 1;
    u8 *src = (u8*)pop(src_n);
    u8 *dst = (u8*)pop(dst_n);
    u32 a = 0; while (a < dst_n && dst[a]) a++;
    u32 z = 0; while (z < src_n && src[z]) z++;
    if (dst_n) {
        u32 cap = dst_n - 1;
        if (a > cap) a = cap;
        if (z > cap - a) z = cap - a;
        if (z) memcpy(dst + a, src, z);
        dst[a + z] = 0;
    }
    push(dst, dst_n);
    cont();
}
