#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32);
#define MAX_BLOCK (1u<<20)
/* stack: token[32], off[u32]; this instr payload becomes new instr payload */
__declspec(dllexport) void run(void){
    u8 token[32]; memcpy(token, pop(32), 32);
    u32 off = *(u32*)pop(4);
    u8 *pay = cvm_payload(); u32 plen = cvm_payload_size();
    u8 *b = cvm_cached_base(); u32 n = cvm_cached_len();
    u32 add = 8 + 32 + plen;
    if (off > n || n + add > MAX_BLOCK) { cont(); return; }
    memmove(b + off + add, b + off, n - off);
    bl_write(b + off, token, 32, pay, plen);
    cvm_cached_set_len(n + add);
    cont();
}
