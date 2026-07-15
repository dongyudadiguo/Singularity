#include "views_common.h"
#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void cvm_vote(const H parent, const H child);
__declspec(dllexport) void run(void) {
    const u8 *id; u32 id_len;
    u32 vi = *(u32*)from(4);
    if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len);
    if (!t) { cont(); return; }
    if (vi == 0xffffffffu) vi = t->active;
    if (vi >= t->count || !t->views[vi].used) { cont(); return; }
    View *v = &t->views[vi];
    if (key_is_tag(v->key)) {
        H kids[256]; H p; memcpy(p, v->key, 32);
        u32 kc = cvm_children(p, kids, 256);
        if (kc > 256) kc = 256;
        H parent; memcpy(parent, v->key, 32);
        for (u32 i = 0; i < kc; i++) {
            if (zero_key(kids[i]) || same_key(kids[i], parent)) continue;
            cvm_vote(parent, kids[i]);
        }
        cont(); return;
    }
    H h; cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 n = cvm_cached_len();
    u32 o = 0;
    H parent; memcpy(parent, v->key, 32);
    while (bl_ok(b, n, o) && !bl_is_end(b + o)) {
        if (bl_tlen(b + o) == 32) {
            H child; memcpy(child, bl_token_c(b + o), 32);
            cvm_vote(parent, child);
        }
        o += bl_instr_size(b + o);
    }
    cont();
}
