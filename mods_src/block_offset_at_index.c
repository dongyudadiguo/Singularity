#include <string.h>
#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){
    u32 index = *(u32*)from(4), off = 0, len = cvm_cached_len();
    u8 *b = cvm_cached_base();
    for (u32 i = 0; i < index && bl_ok(b, len, off) && !bl_is_end(b + off); i++) {
        off += bl_instr_size(b + off);
    }
    memcpy(slot(4), &off, 4);
    cont();
}
