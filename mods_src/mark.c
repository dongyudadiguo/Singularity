extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void mark(void);
__declspec(dllexport) void run(void) {
    mark();
    cont();
}
