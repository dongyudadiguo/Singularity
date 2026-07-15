#include <string.h>
#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
/* stack: index -> token[32] (zero if missing / non-32 token) */
__declspec(dllexport) void run(void){
    u32 index = *(u32*)from(4);
    u8 out[32]; memset(out, 0, 32);
    u8 *b = cvm_cached_base(); u32 len = cvm_cached_len();
    u32 off = 0;
    for (u32 i = 0; i < index && bl_ok(b, len, off) && !bl_is_end(b + off); i++)
        off += bl_instr_size(b + off);
    if (bl_ok(b, len, off) && !bl_is_end(b + off) && bl_tlen(b + off) == 32)
        memcpy(out, bl_token_c(b + off), 32);
    memcpy(slot(32), out, 32);
    cont();
}
