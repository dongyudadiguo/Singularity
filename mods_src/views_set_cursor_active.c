#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
/* payload: id[32]
 * If payload has +u32 cursor after id, use that (legacy).
 * Else pop u32 cursor from stack.
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    u32 cur;
    if (an >= 4) cur = *(u32*)args;
    else cur = *(u32*)from(4);
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t = *tp;
    if (t.active < t.count && t.views[t.active].used) {
        t.views[t.active].cursor = cur;
        store_table(id, id_len, &t);
    }
    cont();
}
