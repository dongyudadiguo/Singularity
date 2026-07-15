#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
/* payload: id[32]; stack: i32 view, i32 row. */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    int row = *(int*)from(4);
    int vi = *(int*)from(4);
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t = *tp;
    if (vi >= 0 && (u32)vi < t.count && t.views[vi].used) {
        t.active = (u32)vi;
        if (row < 0) row = 0;
        t.views[vi].cursor = (u32)row;
        store_table(id, id_len, &t);
    }
    cont();
}
