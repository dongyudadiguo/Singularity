#include <string.h>
#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
/* stack: token[32] -> path[160]
 * Path is tag-graph path when known ("#TAG/#atomic/var_read"), else leaf/hex.
 * Empty if zero token.
 */
__declspec(dllexport) void run(void){
    H token; memcpy(token, from(32), 32);
    char out[160]; memset(out, 0, sizeof(out));
    name_path_of(token, out, sizeof(out));
    memcpy(slot(sizeof(out)), out, sizeof(out));
    cont();
}
