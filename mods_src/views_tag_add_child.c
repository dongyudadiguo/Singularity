#include "views_common.h"
#include <string.h>
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void cvm_edge(const H parent, const H child);
/* stack: parent_key[32] (from active_key), child_key[32] (from sha256_var) */
__declspec(dllexport) void run(void){
    H child, parent;
    memcpy(child, from(32), 32);
    memcpy(parent, from(32), 32);
    if (!zero_key(parent) && !zero_key(child))
        cvm_edge(parent, child);
    cont();
}
