typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
/* stack: x ; payload: f32 lo, f32 hi -> clamp(x,lo,hi) */
__declspec(dllexport) void run(void) {
    float x = *(float*)pop(4);
    float lo = 0.0f, hi = 1.0f;
    if (cvm_payload_size() >= 8) {
        lo = *(float*)cvm_payload();
        hi = *(float*)(cvm_payload() + 4);
    }
    if (x < lo) x = lo;
    if (x > hi) x = hi;
    push(&x, 4);
    cont();
}
