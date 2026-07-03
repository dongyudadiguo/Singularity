typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);

__declspec(dllexport) void run(void) {
    int b = *(int*)pop(4);
    int a = *(int*)pop(4);
    u32 r = (a > b) ? 1 : 0;
    push(&r, 4);
    cont();
}
