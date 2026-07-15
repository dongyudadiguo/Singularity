#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *slot(u32);
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
    memcpy(slot(4), &x, 4); memcpy(slot(4), &y, 4); cont();
}
