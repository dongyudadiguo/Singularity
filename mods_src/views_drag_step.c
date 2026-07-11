#include "views_common.h"
/* args: f32 dx, f32 dy */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t=*tp;
    if (an >= 8 && t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
        t.views[t.dragging].x += *(float*)args;
        t.views[t.dragging].y += *(float*)(args+4);
        store_table(id, id_len, &t);
    }
    cont();
}
