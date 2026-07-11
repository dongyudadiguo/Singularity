#include "views_common.h"
/* payload: id[32]; stack: i32 view, i32 row. */
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id, 0, 0)) { cont(); return; }
    int row = *(int*)pop(4);
    int vi = *(int*)pop(4);
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t = *tp;
    if (vi >= 0 && (u32)vi < t.count && t.views[vi].used) {
        t.active = (u32)vi;
        if (row < 0) row = 0;
        t.views[vi].cursor = (u32)row;
        store_table(id, &t);
    }
    cont();
}
