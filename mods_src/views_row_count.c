#include "views_common.h"
/* payload: id[32] + u32 view_index -> u32 instruction count (end index) */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    u32 n = 0;
    Table *tp = load_table(id);
    if (tp && an >= 4) {
        u32 vi = *(u32*)args;
        if (vi < tp->count && tp->views[vi].used)
            n = block_row_count(&tp->views[vi]);
    }
    push(&n, 4);
    cont();
}
