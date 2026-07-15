#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
/* payload: id[32]
 * stack: key[32], f32 x, f32 y, i32 parent, f32 link_x, f32 link_y
 * Opens or focuses key; starts drag on it. Pushes u32 index or 0xffffffff.
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    float ly = *(float*)from(4), lx = *(float*)from(4);
    int parent = *(int*)from(4);
    float y = *(float*)from(4), x = *(float*)from(4);
    u8 key[32]; memcpy(key, from(32), 32);
    u32 out = 0xffffffffu;
    Table *tp = load_or_empty(id, id_len, 1);
    if (!tp || zero_key(key)) { memcpy(slot(4), &out, 4); cont(); return; }
    Table t = *tp;
    int found = -1;
    for (u32 i = 0; i < t.count; i++)
        if (t.views[i].used && same_key(t.views[i].key, key)) { found = (int)i; break; }
    if (found >= 0) {
        t.active = (u32)found; t.dragging = found;
        t.views[found].x = x; t.views[found].y = y;
        out = (u32)found;
    } else if (t.count < VIEW_MAX) {
        u32 i = t.count++;
        zero_view(&t.views[i]); t.views[i].used = 1;
        memcpy(t.views[i].key, key, 32);
        t.views[i].x = x; t.views[i].y = y;
        t.views[i].parent = parent; t.views[i].linked = 1;
        t.views[i].link_x = lx; t.views[i].link_y = ly;
        t.active = i; t.dragging = (int)i; out = i;
    }
    store_table(id, id_len, &t);
    memcpy(slot(4), &out, 4);
    cont();
}
