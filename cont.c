#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cvm_exec(const H h);

__declspec(dllexport) void cont(void) {
    H token;
    u32 n;

    /* block layout: token[32] + payload_size[u32] + payload[payload_size] */
    memcpy(token, ptr, 32);
    n = *(u32*)(ptr + 32);
    ptr += 32 + 4 + n;
    cvm_exec(token);
}
