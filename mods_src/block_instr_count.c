#include "block_layout.h"
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void*, u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){
    u8 *b = cvm_cached_base(); u32 n = cvm_cached_len(), o = 0, r = 0;
    while (bl_ok(b, n, o) && !bl_is_end(b + o)) {
        o += bl_instr_size(b + o);
        r++;
        if (r > 256) break;
    }
    push(&r, 4);
    cont();
}
