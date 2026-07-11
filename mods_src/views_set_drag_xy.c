#include "views_common.h"
/* stack: x,y absolute for dragging view */
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    float y=*(float*)pop(4), x=*(float*)pop(4);
    Table *tp=load_table(id); if(!tp){cont();return;}
    Table t=*tp;
    if (t.dragging>=0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
        t.views[t.dragging].x=x; t.views[t.dragging].y=y;
        store_table(id,&t);
    }
    cont();
}
