#include <string.h>
#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
/* stack: name[96], query[256] -> u32 0/1  (pop query first) */
__declspec(dllexport) void run(void){
    char *q = (char*)from(256);
    char *n = (char*)from(96);
    u32 r = (n && q && str_prefix_ci_us(n, q)) ? 1u : 0u;
    memcpy(slot(4), &r, 4);
    cont();
}
