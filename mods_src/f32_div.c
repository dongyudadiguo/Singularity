typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
/* stack: a b -> a/b ; b==0 -> 0 */
__declspec(dllexport) void run(void) {
    float b = *(float*)pop(4);
    float a = *(float*)pop(4);
    float r = (b != 0.0f) ? (a / b) : 0.0f;
    push(&r, 4);
    cont();
}
