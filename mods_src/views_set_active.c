#include "views_common.h"
/* args: u32 index */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t = *tp;
    if (an >= 4) {
        u32 idx = *(u32*)args;
        if (idx < t.count && t.views[idx].used) t.active = idx;
        store_table(id, &t);
    }
    cont();
}
