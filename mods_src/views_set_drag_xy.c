#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
/* stack: x,y absolute for dragging view */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    float y=*(float*)from(4), x=*(float*)from(4);
    Table *tp=load_table(id, id_len); if(!tp){cont();return;}
    Table t=*tp;
    if (t.dragging>=0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
        t.views[t.dragging].x=x; t.views[t.dragging].y=y;
        store_table(id, id_len, &t);
    }
    cont();
}
