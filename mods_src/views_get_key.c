#include "views_common.h"
/* args: u32 index -> key[32] */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    u8 key[32]; memset(key,0,32);
    Table *tp = load_table(id, id_len);
    if (tp && an >= 4) {
        u32 idx=*(u32*)args;
        if (idx < tp->count && tp->views[idx].used) memcpy(key, tp->views[idx].key, 32);
    }
    push(key,32); cont();
}
