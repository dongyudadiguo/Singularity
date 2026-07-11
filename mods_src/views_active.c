#include "views_common.h"
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id, 0, 0)) { cont(); return; }
    Table *tp = load_table(id);
    u32 z = tp ? tp->active : 0;
    push(&z, 4); cont();
}
