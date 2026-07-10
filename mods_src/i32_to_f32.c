typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
__declspec(dllexport) void run(void) {
    int v = *(int*)pop(4);
    float f = (float)v;
    push(&f, 4);
    cont();
}
