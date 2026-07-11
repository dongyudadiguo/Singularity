#include "views_common.h"
/* init: args key[32], f32 x, f32 y — reset table with one view */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an) || an < 40) { cont(); return; }
    cvm_var_set(id, 32, (u32)sizeof(Table));
    Table t; memset(&t, 0, sizeof(t));
    t.dragging = -1; t.count = 1; t.active = 0;
    zero_view(&t.views[0]);
    t.views[0].used = 1;
    memcpy(t.views[0].key, args, 32);
    t.views[0].x = *(float*)(args+32);
    t.views[0].y = *(float*)(args+36);
    store_table(id, &t);
    cont();
}
