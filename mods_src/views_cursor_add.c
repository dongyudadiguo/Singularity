#include "views_common.h"
/* args: i32 delta on active, clamped to block rows + end */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    Table *tp=load_table(id, id_len); if(!tp){cont();return;}
    Table t=*tp;
    if (an>=4 && t.active < t.count && t.views[t.active].used) {
        int d=*(int*)args;
        int cur=(int)t.views[t.active].cursor + d;
        u32 maxr=block_row_count(&t.views[t.active]);
        if (cur<0) cur=0;
        if ((u32)cur>maxr) cur=(int)maxr;
        t.views[t.active].cursor=(u32)cur;
        store_table(id, id_len, &t);
    }
    cont();
}
