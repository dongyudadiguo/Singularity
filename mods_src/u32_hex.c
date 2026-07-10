typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
__declspec(dllexport) void run(void){static const char x[]="0123456789abcdef";u32 v=*(u32*)pop(4);char out[8];for(int i=7;i>=0;i--){out[i]=x[v&15];v>>=4;}push(out,8);cont();}
