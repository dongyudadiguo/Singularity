#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
/* stack: v; payload: i32 lo, i32 hi -> clamp(v,lo,hi) */
__declspec(dllexport) void run(void){
    int v = *(int*)from(4);
    int lo = 0, hi = 0x7fffffff;
    u8 *p = cvm_payload();
    if (cvm_payload_size() >= 8) { lo = *(int*)p; hi = *(int*)(p+4); }
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    memcpy(slot(4), &v, 4); cont();
}
