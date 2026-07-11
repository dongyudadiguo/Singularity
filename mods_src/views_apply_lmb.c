#include "views_common.h"
/* stack: mx,my; payload: id + title_h,row_h,row_count
 * title -> set active + dragging (LMB title drag)
 * row   -> select_row, clear dragging
 * miss  -> clear dragging
 * pushes u32 handled.
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)pop(4), mx=*(float*)pop(4);
    float title_h=32.f, row_h=24.f; u32 row_count=256;
    if (an>=4) title_h=*(float*)args;
    if (an>=8) row_h=*(float*)(args+4);
    if (an>=12) row_count=*(u32*)(args+8);
    u32 handled=0;
    Table *tp=load_table(id, id_len); if(!tp){ push(&handled,4); cont(); return; }
    Table t=*tp;
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float width=title_text_width((u32)i,v);
        if (mx>=v->x && mx<v->x+width && my>=v->y-title_h && my<v->y) {
            t.active=(u32)i;
            t.dragging=i;
            handled=1;
            store_table(id, id_len, &t);
            push(&handled,4); cont(); return;
        }
    }
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float rel=my-v->y; if(rel<0) continue;
        int row=(int)(rel/row_h);
        if (row<0 || (u32)row>row_count) continue;
        float width=row_hit_width(v,row);
        if (mx<v->x || mx>=v->x+width) continue;
        t.active=(u32)i;
        t.views[i].cursor=(u32)row;
        t.dragging=-1; /* row select is not a drag */
        handled=1;
        store_table(id, id_len, &t);
        push(&handled,4); cont(); return;
    }
    /* miss: drop any stale drag */
    if (t.dragging != -1) {
        t.dragging = -1;
        store_table(id, id_len, &t);
    }
    push(&handled,4); cont();
}
