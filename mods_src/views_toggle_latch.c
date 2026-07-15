#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
/* stack: u32 view_index (0xffffffff = active). Toggle pad0 bit0. */
__declspec(dllexport) void run(void) {
    const u8 *id; u32 id_len;
    u32 vi = *(u32*)from(4);
    if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *tp = load_table(id, id_len);
    if (!tp) { cont(); return; }
    Table t = *tp;
    if (vi == 0xffffffffu) vi = t.active;
    if (vi >= t.count || !t.views[vi].used) { cont(); return; }
    t.views[vi].pad0 ^= 1u;
    store_table(id, id_len, &t);
    cont();
}
