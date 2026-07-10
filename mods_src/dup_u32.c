typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
__declspec(dllexport) void run(void) {
    u32 v = *(u32*)pop(4);
    push(&v, 4);
    push(&v, 4);
    cont();
}
