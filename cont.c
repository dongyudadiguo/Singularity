#include <string.h>

/*
 * block layout (data -> data):
 *   token_len[u32] + token[token_len] + payload_len[u32] + payload[payload_len]
 *   ...
 *   token_len == 0  (4 zero bytes) ends the block
 *
 * ptr points at the current instruction header (token_len).
 */

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cvm_exec_instr(void);
extern __declspec(dllimport) int cvm_ret(void);

__declspec(dllexport) void cont(void) {
    u32 tlen, plen;
    if (!ptr) return;
    tlen = *(u32 *)ptr;
    if (tlen > (1u << 20)) return;
    plen = *(u32 *)(ptr + 4 + tlen);
    if (plen > (1u << 20)) return;
    ptr += 8 + tlen + plen;

    tlen = *(u32 *)ptr;
    if (tlen == 0) {
        if (cvm_ret()) cont();
        return;
    }
    cvm_exec_instr();
}
