#include "views_common.h"
/* payload: id[32]; stack: i32 index (>=0). Sets active+dragging if valid. */
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id, 0, 0)) { cont(); return; }
    int idx = *(int*)pop(4);
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t = *tp;
    if (idx >= 0 && (u32)idx < t.count && t.views[idx].used) {
        t.active = (u32)idx;
        t.dragging = idx;
        store_table(id, &t);
    }
    cont();
}
