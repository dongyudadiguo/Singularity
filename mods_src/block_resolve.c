#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
/* stack: key[32] -> resolves into cache (side effect). */
__declspec(dllexport) void run(void){
    H k, h; memcpy(k, pop(32), 32);
    cvm_resolve_payload_hash(k, h);
    cont();
}
