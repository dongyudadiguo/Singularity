#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
/* payload: id[32]; stack: i32 index (>=0). Sets active+dragging if valid. */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    int idx = *(int*)from(4);
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t = *tp;
    if (idx >= 0 && (u32)idx < t.count && t.views[idx].used) {
        t.active = (u32)idx;
        t.dragging = idx;
        store_table(id, id_len, &t);
    }
    cont();
}
