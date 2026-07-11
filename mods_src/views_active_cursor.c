#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    Table *tp=load_table(id);
    u32 cur=0;
    if (tp && tp->active < tp->count && tp->views[tp->active].used)
        cur = tp->views[tp->active].cursor;
    push(&cur,4); cont();
}
