#include "views_common.h"
/* args: u32 index, f32 dx, f32 dy */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp=load_table(id, id_len); if(!tp){cont();return;}
    Table t=*tp;
    if (an>=12) {
        u32 idx=*(u32*)args;
        if (idx<t.count && t.views[idx].used) {
            t.views[idx].x += *(float*)(args+4);
            t.views[idx].y += *(float*)(args+8);
            store_table(id, id_len, &t);
        }
    }
    cont();
}
