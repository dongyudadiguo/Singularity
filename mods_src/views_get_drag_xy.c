#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    float x=0,y=0;
    Table *tp=load_table(id);
    if (tp && tp->dragging>=0 && (u32)tp->dragging < tp->count && tp->views[tp->dragging].used) {
        x=tp->views[tp->dragging].x; y=tp->views[tp->dragging].y;
    }
    push(&x,4); push(&y,4); cont();
}
