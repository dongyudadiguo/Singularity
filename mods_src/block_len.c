typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void) {
    u32 n = cvm_cached_len();
    if (n >= 32) n -= 32; /* offset of final zero token */
    else n = 0;
    push(&n, 4);
    cont();
}
