#include <string.h>
#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32);
/* -> u32 count */
__declspec(dllexport) void run(void){
    name_load();
    memcpy(slot(4), &g_name_n, 4);
    cont();
}
