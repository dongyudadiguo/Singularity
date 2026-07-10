typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
/* stack: x y rx ry rw rh  (all f32) -> u32 0/1
 * hit if x in [rx,rx+rw) and y in [ry,ry+rh)
 */
__declspec(dllexport) void run(void) {
    float rh = *(float*)pop(4);
    float rw = *(float*)pop(4);
    float ry = *(float*)pop(4);
    float rx = *(float*)pop(4);
    float y  = *(float*)pop(4);
    float x  = *(float*)pop(4);
    u32 v = (x >= rx && x < rx + rw && y >= ry && y < ry + rh) ? 1u : 0u;
    push(&v, 4);
    cont();
}
