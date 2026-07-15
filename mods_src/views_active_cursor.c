#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *slot(u32);
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *tp=load_table(id, id_len);
    u32 cur=0;
    if (tp && tp->active < tp->count && tp->views[tp->active].used)
        cur = tp->views[tp->active].cursor;
    memcpy(slot(4), &cur, 4); cont();
}
