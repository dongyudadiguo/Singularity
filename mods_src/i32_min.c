typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
__declspec(dllexport) void run(void){
    int b=*(int*)pop(4), a=*(int*)pop(4);
    int r = a < b ? a : b; push(&r,4); cont();
}
