typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
/* stack: a b -> a*b */
__declspec(dllexport) void run(void) {
    float b = *(float*)pop(4);
    float a = *(float*)pop(4);
    float r = a * b;
    push(&r, 4);
    cont();
}
