#include "views_common.h"
/* stack mx,my; payload id+title_h,row_h,row_count
 * title -> active+drag; row -> open target key at mouse (hash-carrier aware).
 * pushes u32 handled.
 */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id,&args,&an)) { cont(); return; }
    float my=*(float*)pop(4), mx=*(float*)pop(4);
    float title_h=32.f, row_h=24.f; u32 row_count=256;
    if (an>=4) title_h=*(float*)args;
    if (an>=8) row_h=*(float*)(args+4);
    if (an>=12) row_count=*(u32*)(args+8);
    u32 handled=0;
    Table *tp=load_or_empty(id,0); if(!tp){ push(&handled,4); cont(); return; }
    Table t=*tp;
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float width=title_text_width((u32)i,v);
        if (mx>=v->x && mx<v->x+width && my>=v->y-title_h && my<v->y) {
            t.active=(u32)i; t.dragging=i; handled=1; store_table(id,&t); push(&handled,4); cont(); return;
        }
    }
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float rel=my-v->y; if(rel<0) continue;
        int row=(int)(rel/row_h);
        if (row<0 || (u32)row>=row_count) continue;
        float width=row_hit_width(v,row);
        if (mx<v->x || mx>=v->x+width) continue;
        u32 o=block_row_offset(v,(u32)row);
        u8 key[32]; instr_open_key(cvm_cached_base(), cvm_cached_len(), o, key);
        if (zero_key(key)) break;
        int found=-1;
        for (u32 j=0;j<t.count;j++) if (t.views[j].used && same_key(t.views[j].key,key)) { found=(int)j; break; }
        if (found>=0) {
            t.active=(u32)found; t.dragging=found;
            t.views[found].x=mx; t.views[found].y=my;
        } else if (t.count < VIEW_MAX) {
            u32 ni=t.count++;
            zero_view(&t.views[ni]); t.views[ni].used=1;
            memcpy(t.views[ni].key,key,32);
            t.views[ni].x=mx; t.views[ni].y=my;
            t.views[ni].parent=i; t.views[ni].linked=1;
            t.views[ni].link_x = width*0.5f; if (t.views[ni].link_x<24.f) t.views[ni].link_x=24.f;
            t.views[ni].link_y = (float)row*row_h + 10.f;
            t.active=ni; t.dragging=(int)ni;
        }
        handled=1; break;
    }
    store_table(id,&t); push(&handled,4); cont();
}
