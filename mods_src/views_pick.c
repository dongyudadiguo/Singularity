#include "views_common.h"
/* stack: f32 mx, my
 * payload: id[32] + f32 title_h + f32 row_h + u32 row_count
 * pushes: i32 view (-1 none), i32 row (-1 none), i32 zone (0 none, 1 title, 2 row)
 * Pure hit test — no state mutation. Recipes decide set_active/open/drag.
 */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    float my = *(float*)pop(4), mx = *(float*)pop(4);
    float title_h = 32.0f, row_h = 24.0f; u32 row_count = 256;
    if (an >= 4) title_h = *(float*)args;
    if (an >= 8) row_h = *(float*)(args + 4);
    if (an >= 12) row_count = *(u32*)(args + 8);
    int vhit = -1, rhit = -1, zone = 0;
    Table *tp = load_table(id);
    if (tp) {
        Table t = *tp;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float width = title_text_width((u32)i, v);
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                vhit = i; rhit = -1; zone = 1; break;
            }
        }
        if (!zone) {
            for (int i = (int)t.count - 1; i >= 0; i--) {
                View *v = &t.views[i];
                if (!v->used) continue;
                float rel = my - v->y;
                if (rel < 0.0f) continue;
                int row = (int)(rel / row_h);
                if (row < 0 || (u32)row > row_count) continue;
                float width = row_hit_width(v, row);
                if (mx < v->x || mx >= v->x + width) continue;
                vhit = i; rhit = row; zone = 2; break;
            }
        }
    }
    push(&vhit, 4); push(&rhit, 4); push(&zone, 4);
    cont();
}
