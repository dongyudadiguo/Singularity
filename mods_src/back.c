extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void back(void);
__declspec(dllexport) void run(void) {
    back();
    cont();
}
