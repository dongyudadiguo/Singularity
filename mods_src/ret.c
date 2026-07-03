extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) int cvm_ret(void);

__declspec(dllexport) void run(void) {
    if (cvm_ret()) cont();
}
