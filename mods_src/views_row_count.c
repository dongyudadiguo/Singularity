#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *slot(u32);
/* payload: id[32] + u32 view_index -> u32 instruction count (end index) */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    u32 n = 0;
    Table *tp = load_table(id, id_len);
    if (tp && an >= 4) {
        u32 vi = *(u32*)args;
        if (vi < tp->count && tp->views[vi].used)
            n = block_row_count(&tp->views[vi]);
    }
    memcpy(slot(4), &n, 4);
    cont();
}
