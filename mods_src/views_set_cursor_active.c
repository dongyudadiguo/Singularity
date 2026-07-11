#include "views_common.h"
/* args: u32 cursor on active view */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    Table *tp=load_table(id); if(!tp){cont();return;}
    Table t=*tp;
    if (an>=4 && t.active < t.count && t.views[t.active].used) {
        t.views[t.active].cursor = *(u32*)args;
        store_table(id,&t);
    }
    cont();
}
