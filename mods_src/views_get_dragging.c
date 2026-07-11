#include "views_common.h"
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *tp=load_table(id, id_len);
    int z = tp ? tp->dragging : -1;
    push(&z,4); cont();
}
