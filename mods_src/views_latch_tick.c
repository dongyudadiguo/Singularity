#include "views_common.h"
extern __declspec(dllimport) int cvm_key_dirty(const H key);
extern __declspec(dllimport) void cvm_flush_key(const H key);
extern __declspec(dllimport) void cvm_heat_tick(void);

/* Per-frame: decay heat; auto-uset any view with latch bit set when dirty.
 * View.pad0 bit0 = latch (合闸).
 */
__declspec(dllexport) void run(void) {
    const u8 *id; u32 id_len;
    cvm_heat_tick();
    if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len);
    if (!t) { cont(); return; }
    for (u32 i = 0; i < t->count; i++) {
        View *v = &t->views[i];
        if (!v->used) continue;
        if (!(v->pad0 & 1u)) continue;
        if (key_is_tag(v->key)) continue;
        if (cvm_key_dirty(v->key)) cvm_flush_key(v->key);
    }
    cont();
}
