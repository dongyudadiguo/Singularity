typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
__declspec(dllexport) void run(void) {
    float x = *(float*)pop(4);
    x = -x;
    push(&x, 4);
    cont();
}
