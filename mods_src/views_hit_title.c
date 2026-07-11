#include "views_common.h"
/* stack: mx,my; args: f32 title_h -> i32 hit */
__declspec(dllexport) void run(void){
    H id; const u8 *args; u32 an;
    if (!payload_id(id, &args, &an)) { cont(); return; }
    float my=*(float*)pop(4), mx=*(float*)pop(4);
    float title_h = an>=4 ? *(float*)args : 32.0f;
    int hit=-1;
    Table *tp=load_table(id);
    if (tp) {
        Table t=*tp;
        for (int i=(int)t.count-1;i>=0;i--) {
            View *v=&t.views[i]; if(!v->used) continue;
            float width=title_text_width((u32)i,v);
            if (mx>=v->x && mx<v->x+width && my>=v->y-title_h && my<v->y) { hit=i; break; }
        }
    }
    push(&hit,4); cont();
}
