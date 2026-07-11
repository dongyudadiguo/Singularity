#include "views_common.h"
/* args: u32 index -> u32 */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    u32 cur=0;
    Table *tp = load_table(id);
    if (tp && an >= 4) {
        u32 idx=*(u32*)args;
        if (idx < tp->count && tp->views[idx].used) cur = tp->views[idx].cursor;
    }
    push(&cur,4); cont();
}
