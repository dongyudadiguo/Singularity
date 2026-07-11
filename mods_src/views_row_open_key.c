#include "views_common.h"
/* payload: views_var[32]; stack: u32 view_index, u32 row -> key[32] (open target) */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    u32 row = *(u32*)pop(4);
    u32 vi = *(u32*)pop(4);
    u8 key[32]; memset(key, 0, 32);
    Table *tp = load_table(id, id_len);
    if (tp && vi < tp->count && tp->views[vi].used) {
        u32 o = block_row_offset(&tp->views[vi], row);
        instr_open_key(cvm_cached_base(), cvm_cached_len(), o, key);
    }
    push(key, 32);
    cont();
}
