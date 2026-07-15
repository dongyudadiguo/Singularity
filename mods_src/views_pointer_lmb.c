#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
/* stack mx,my; args optional title_h,pad,row_h,row_count
 * title -> active + dragging; row -> select (no drag).
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)from(4), mx=*(float*)from(4);
    float title_h=32.0f, row_h=24.0f; u32 row_count=256;
    if (an>=4) title_h=*(float*)args;
    if (an>=12) row_h=*(float*)(args+8);
    else if (an>=8) row_h=*(float*)(args+4);
    if (an>=16) row_count=*(u32*)(args+12);
    else if (an>=12) row_count=*(u32*)(args+8);
    int handled=0;
    Table *tp=load_or_empty(id, id_len, 0);
    if (!tp) { memcpy(slot(4), &handled, 4); cont(); return; }
    Table t=*tp;
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float width=title_text_width((u32)i,v);
        if (mx>=v->x && mx<v->x+width && my>=v->y-title_h && my<v->y) {
            t.active=(u32)i; t.dragging=i; handled=1; break;
        }
    }
    if (!handled) {
        for (int i=(int)t.count-1;i>=0;i--) {
            View *v=&t.views[i]; if(!v->used) continue;
            float rel=my-v->y; if(rel<0) continue;
            int row=(int)(rel/row_h);
            if (row<0 || (u32)row>row_count) continue;
            float width=row_hit_width(v,row);
            if (mx<v->x || mx>=v->x+width) continue;
            t.active=(u32)i; t.views[i].cursor=(u32)row; t.dragging=-1; handled=1; break;
        }
    }
    store_table(id, id_len, &t); memcpy(slot(4), &handled, 4); cont();
}
