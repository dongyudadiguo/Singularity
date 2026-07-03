extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void cvm_scope_start(void);

__declspec(dllexport) void run(void) {
    cvm_scope_start();
    cont();
}