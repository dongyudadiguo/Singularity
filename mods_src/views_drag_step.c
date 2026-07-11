#include "views_common.h"
/* args: f32 dx, f32 dy */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t=*tp;
    if (an >= 8 && t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
        t.views[t.dragging].x += *(float*)args;
        t.views[t.dragging].y += *(float*)(args+4);
        store_table(id,&t);
    }
    cont();
}
