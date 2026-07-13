#include "views_common.h"
extern __declspec(dllimport) void cvm_flush_key(const H key);

/* stack: u32 view_index (0xffffffff = active) */
__declspec(dllexport) void run(void) {
    const u8 *id; u32 id_len;
    u32 vi = *(u32*)pop(4);
    if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len);
    if (!t) { cont(); return; }
    if (vi == 0xffffffffu) vi = t->active;
    if (vi >= t->count || !t->views[vi].used) { cont(); return; }
    if (key_is_tag(t->views[vi].key)) { cont(); return; }
    cvm_flush_key(t->views[vi].key);
    cont();
}
