typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
__declspec(dllexport) void run(void) {
    u32 b = *(u32*)pop(4);
    u32 a = *(u32*)pop(4);
    push(&b, 4);
    push(&a, 4);
    cont();
}
