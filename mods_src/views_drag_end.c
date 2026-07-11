#include "views_common.h"
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *tp = load_table(id, id_len); if (!tp) { cont(); return; }
    Table t=*tp; t.dragging = -1; store_table(id, id_len, &t); cont();
}
