#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
/* payload: n[u32] (buffer size; default 256)
 * stack: s[n] -> s[n] with last C-string char removed
 */
__declspec(dllexport) void run(void){
    u32 n = cvm_payload_size() >= 4 ? *(u32*)cvm_payload() : 256;
    if (!n) n = 1;
    u8 *s = (u8*)from(n);
    u32 z = 0; while (z < n && s[z]) z++;
    if (z) s[z - 1] = 0;
    memcpy(slot(n), s, n);
    cont();
}
