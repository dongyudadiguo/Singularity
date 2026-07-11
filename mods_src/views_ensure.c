#include "views_common.h"
/* ensure: args key[32], f32 x, f32 y
 * - if table empty: seed program view + #TAG explorer
 * - if table non-empty but no #TAG view: append explorer
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp = load_or_empty(id, id_len, 1);
    if (!tp) { cont(); return; }
    Table t = *tp;
    int dirty = 0;

    if (an >= 40 && t.count == 0) {
        t.dragging = -1;
        t.count = 0; t.active = 0;

        zero_view(&t.views[0]);
        t.views[0].used = 1;
        memcpy(t.views[0].key, args, 32);
        t.views[0].x = *(float*)(args+32);
        t.views[0].y = *(float*)(args+36);
        t.count = 1;

        if (t.count < VIEW_MAX) {
            u32 i = t.count++;
            zero_view(&t.views[i]);
            t.views[i].used = 1;
            tag_root_key(t.views[i].key);
            t.views[i].x = *(float*)(args+32) + 360.0f;
            t.views[i].y = *(float*)(args+36);
            t.views[i].parent = -1;
            t.views[i].linked = 0;
        }
        t.active = 0;
        dirty = 1;
    } else {
        /* ensure a #TAG explorer view exists */
        u8 root[32]; tag_root_key(root);
        int found = 0;
        for (u32 i = 0; i < t.count; i++) {
            if (t.views[i].used && same_key(t.views[i].key, root)) { found = 1; break; }
        }
        if (!found && t.count < VIEW_MAX) {
            u32 i = t.count++;
            zero_view(&t.views[i]);
            t.views[i].used = 1;
            memcpy(t.views[i].key, root, 32);
            t.views[i].x = 400.0f;
            t.views[i].y = 70.0f;
            t.views[i].parent = -1;
            t.views[i].linked = 0;
            dirty = 1;
        }
    }
    if (dirty) store_table(id, id_len, &t);
    cont();
}
