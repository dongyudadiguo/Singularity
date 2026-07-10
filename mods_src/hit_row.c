typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
/* stack: y origin_y row_h count  (f32,f32,f32,u32) -> i32 index or -1 */
__declspec(dllexport) void run(void) {
    u32 count = *(u32*)pop(4);
    float row_h = *(float*)pop(4);
    float origin_y = *(float*)pop(4);
    float y = *(float*)pop(4);
    int idx = -1;
    if (row_h > 0.0f && count > 0) {
        float rel = y - origin_y;
        if (rel >= 0.0f) {
            int i = (int)(rel / row_h);
            if (i >= 0 && (u32)i < count) idx = i;
        }
    }
    push(&idx, 4);
    cont();
}
