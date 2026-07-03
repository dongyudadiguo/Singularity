extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void cvm_cache_flush(void);
__declspec(dllexport) void run(void) {
    cvm_cache_flush();
    cont();
}
