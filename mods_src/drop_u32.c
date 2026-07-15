typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
__declspec(dllexport) void run(void) {
    from(4);
    cont();
}
