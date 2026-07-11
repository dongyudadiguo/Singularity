#include "views_common.h"
/* args: u32 index, u32 cursor */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t=*tp;
    if (an >= 8) {
        u32 idx=*(u32*)args;
        if (idx < t.count && t.views[idx].used) {
            t.views[idx].cursor = *(u32*)(args+4);
            store_table(id, id_len, &t);
        }
    }
    cont();
}
