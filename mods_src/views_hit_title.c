#include <string.h>
#include "views_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
/* stack: mx,my; args: f32 title_h -> i32 hit */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)from(4), mx=*(float*)from(4);
    float title_h = an>=4 ? *(float*)args : 32.0f;
    int hit=-1;
    Table *tp=load_table(id, id_len);
    if (tp) {
        Table t=*tp;
        for (int i=(int)t.count-1;i>=0;i--) {
            View *v=&t.views[i]; if(!v->used) continue;
            float width=title_text_width((u32)i,v);
            float dx=view_draw_x(&t,(u32)i);
            if (mx>=dx && mx<dx+width && my>=v->y-title_h && my<v->y) { hit=i; break; }
        }
    }
    memcpy(slot(4), &hit, 4); cont();
}
