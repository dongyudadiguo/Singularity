#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id, 0, 0)) { cont(); return; }
    Table *tp = load_table(id); if (!tp) { cont(); return; }
    Table t=*tp; t.dragging = -1; store_table(id,&t); cont();
}
