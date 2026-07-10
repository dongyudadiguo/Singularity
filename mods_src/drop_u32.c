typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
__declspec(dllexport) void run(void) {
    pop(4);
    cont();
}
