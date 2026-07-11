#include "views_common.h"
/* args: key[32], f32 x,y, i32 parent, f32 lx,ly -> u32 index or 0xffffffff */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    u32 out = 0xffffffffu;
    Table *tp = load_or_empty(id, 1);
    if (!tp || an < 52) { push(&out,4); cont(); return; }
    Table t=*tp;
    const u8 *key = args;
    float x = *(float*)(args+32), y=*(float*)(args+36);
    int parent = *(int*)(args+40);
    float lx=*(float*)(args+44), ly=*(float*)(args+48);
    int found=-1;
    for (u32 i=0;i<t.count;i++) if (t.views[i].used && same_key(t.views[i].key,key)) { found=(int)i; break; }
    if (found>=0) { t.active=(u32)found; t.dragging=found; out=(u32)found; }
    else if (t.count < VIEW_MAX) {
        u32 i=t.count++;
        zero_view(&t.views[i]); t.views[i].used=1;
        memcpy(t.views[i].key,key,32);
        t.views[i].x=x; t.views[i].y=y;
        t.views[i].parent=parent; t.views[i].linked=1;
        t.views[i].link_x=lx; t.views[i].link_y=ly;
        t.active=i; t.dragging=(int)i; out=i;
    }
    store_table(id,&t); push(&out,4); cont();
}
