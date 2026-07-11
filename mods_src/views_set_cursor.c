#include "views_common.h"
/* args: u32 index, u32 cursor */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t=*tp;
    if (an >= 8) {
        u32 idx=*(u32*)args;
        if (idx < t.count && t.views[idx].used) {
            t.views[idx].cursor = *(u32*)(args+4);
            store_table(id,&t);
        }
    }
    cont();
}
