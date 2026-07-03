typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
__declspec(dllexport) void run(void) {
    int *m = (int*)pop(16);
    int y = m[1];
    push(&y, 4);
    cont();
}
