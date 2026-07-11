#include "views_common.h"
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *tp=load_table(id, id_len);
    u32 cur=0;
    if (tp && tp->active < tp->count && tp->views[tp->active].used)
        cur = tp->views[tp->active].cursor;
    push(&cur,4); cont();
}
