#include "views_common.h"
/* args: u32 index -> f32 x,y */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float x=0,y=0;
    Table *tp = load_table(id, id_len);
    if (tp && an >= 4) {
        u32 idx = *(u32*)args;
        if (idx < tp->count && tp->views[idx].used) { x=tp->views[idx].x; y=tp->views[idx].y; }
    }
    push(&x,4); push(&y,4); cont();
}
