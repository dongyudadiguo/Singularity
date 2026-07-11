#include "views_common.h"
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    u8 key[32]; memset(key,0,32);
    Table *tp=load_table(id, id_len);
    if (tp && tp->active < tp->count && tp->views[tp->active].used)
        memcpy(key, tp->views[tp->active].key, 32);
    push(key,32); cont();
}
