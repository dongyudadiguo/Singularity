#include "views_common.h"
/* ensure: args key[32], f32 x, f32 y — seed view0 if table empty */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp = load_or_empty(id, id_len, 1);
    if (!tp) { cont(); return; }
    Table t = *tp;
    if (an >= 40 && t.count == 0) {
        t.dragging = -1;
        t.count = 1; t.active = 0;
        zero_view(&t.views[0]);
        t.views[0].used = 1;
        memcpy(t.views[0].key, args, 32);
        t.views[0].x = *(float*)(args+32);
        t.views[0].y = *(float*)(args+36);
        store_table(id, id_len, &t);
    }
    cont();
}
