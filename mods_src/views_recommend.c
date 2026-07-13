#include "views_common.h"
extern __declspec(dllimport) void cvm_vote(const H parent, const H child);

/* Vote edges: view.key -> each instruction token in its block.
 * stack: u32 view_index (or 0xffffffff = active)
 */
__declspec(dllexport) void run(void) {
    const u8 *id; u32 id_len;
    u32 vi = *(u32*)pop(4);
    if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len);
    if (!t) { cont(); return; }
    if (vi == 0xffffffffu) vi = t->active;
    if (vi >= t->count || !t->views[vi].used) { cont(); return; }
    View *v = &t->views[vi];
    if (key_is_tag(v->key)) { cont(); return; }
    {
        H h; cvm_resolve_payload_hash(v->key, h);
        u8 *b = cvm_cached_base();
        u32 n = cvm_cached_len();
        u32 o = 0;
        H parent; memcpy(parent, v->key, 32);
        while (o + 36 <= n && !zero_key(b + o)) {
            u32 pn = *(u32*)(b + o + 32);
            if (o + 36 + pn > n) break;
            H child; memcpy(child, b + o, 32);
            cvm_vote(parent, child);
            o += 36 + pn;
            if (o > (1u << 20)) break;
        }
    }
    cont();
}
