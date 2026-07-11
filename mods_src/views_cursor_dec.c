#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    Table *tp=load_table(id); if(!tp){cont();return;}
    Table t=*tp;
    if (t.active < t.count && t.views[t.active].used) {
        if (t.views[t.active].cursor>0) t.views[t.active].cursor--;
        store_table(id,&t);
    }
    cont();
}
