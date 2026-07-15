#include "block_layout.h"
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32);
#define MAX_BLOCK (1u<<20)
/* stack: token[32], off[u32] -> insert empty-payload instr at off */
__declspec(dllexport) void run(void){
    u8 token[32]; memcpy(token, pop(32), 32);
    u32 off = *(u32*)pop(4);
    u32 nz = 0; for (int i=0;i<32;i++) nz |= token[i];
    u8 *b = cvm_cached_base(); u32 n = cvm_cached_len();
    if (!nz || off > n) { cont(); return; }
    u32 add = 8 + 32 + 0; /* tlen + token32 + plen0 */
    if (n + add > MAX_BLOCK) { cont(); return; }
    memmove(b + off + add, b + off, n - off);
    bl_write(b + off, token, 32, 0, 0);
    cvm_cached_set_len(n + add);
    cont();
}
