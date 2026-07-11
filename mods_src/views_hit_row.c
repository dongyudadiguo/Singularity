#include "views_common.h"
/* stack: mx,my; args: f32 row_h, u32 row_count -> i32 vhit, i32 rhit */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)pop(4), mx=*(float*)pop(4);
    float row_h=24.0f; u32 row_count=64;
    if (an>=4) row_h=*(float*)args;
    if (an>=8) row_count=*(u32*)(args+4);
    int vhit=-1, rhit=-1;
    Table *tp=load_table(id, id_len);
    if (tp) {
        Table t=*tp;
        for (int i=(int)t.count-1;i>=0;i--) {
            View *v=&t.views[i]; if(!v->used) continue;
            float rel=my-v->y; if(rel<0) continue;
            int row=(int)(rel/row_h);
            if (row<0 || (u32)row>=row_count) continue;
            float width=row_hit_width(v,row);
            if (mx<v->x || mx>=v->x+width) continue;
            vhit=i; rhit=row; break;
        }
    }
    push(&vhit,4); push(&rhit,4); cont();
}
