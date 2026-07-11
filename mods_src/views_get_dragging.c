#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    Table *tp=load_table(id);
    int z = tp ? tp->dragging : -1;
    push(&z,4); cont();
}
