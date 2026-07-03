extern __declspec(dllimport) void cvm_reexec(void);

__declspec(dllexport) void run(void) {
    cvm_reexec();
}
