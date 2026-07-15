#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
__declspec(dllexport) void run(void) {
    u32 n = cvm_payload_size();
    u8 *p = cvm_payload();
    void *s = slot(n);
    if (p && n) memcpy(s, p, n);
    cont();
}
