#include <string.h>

/*
 * block layout:
 *   token[32] + payload_size[u32] + payload[payload_size]
 *   ...
 *   zero_token[32]
 *
 * ptr always points at the currently running instruction so payload mods can
 * read cvm_payload()/cvm_payload_size(). cont() skips the current instruction
 * and dispatches the next one. A 32-byte zero token is the block terminator.
 */

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cvm_exec(const H h);
extern __declspec(dllimport) int cvm_ret(void);

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

__declspec(dllexport) void cont(void) {
    H token;
    u32 n;

    n = *(u32*)(ptr + 32);
    ptr += 32 + 4 + n;

    if (zero32(ptr)) {
        if (cvm_ret()) cont();
        return;
    }

    memcpy(token, ptr, 32);
    cvm_exec(token);
}
