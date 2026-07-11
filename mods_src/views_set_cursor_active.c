#include "views_common.h"
/* payload: id[32]
 * If payload has +u32 cursor after id, use that (legacy).
 * Else pop u32 cursor from stack.
 */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    u32 cur;
    if (an >= 4) cur = *(u32*)args;
    else cur = *(u32*)pop(4);
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t = *tp;
    if (t.active < t.count && t.views[t.active].used) {
        t.views[t.active].cursor = cur;
        store_table(id, &t);
    }
    cont();
}
