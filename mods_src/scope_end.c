extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void cvm_scope_end(void);

__declspec(dllexport) void run(void) {
    cvm_scope_end();
    cont();
}