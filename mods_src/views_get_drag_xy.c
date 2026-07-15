#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *slot(u32);
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    float x=0,y=0;
    Table *tp=load_table(id, id_len);
    if (tp && tp->dragging>=0 && (u32)tp->dragging < tp->count && tp->views[tp->dragging].used) {
        x=tp->views[tp->dragging].x; y=tp->views[tp->dragging].y;
    }
    memcpy(slot(4), &x, 4); memcpy(slot(4), &y, 4); cont();
}
