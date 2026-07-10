typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
static int zero32(const u8*p){for(int i=0;i<32;i++)if(p[i])return 0;return 1;}
__declspec(dllexport) void run(void){u32 index=*(u32*)pop(4),off=0,len=cvm_cached_len();u8*b=cvm_cached_base();for(u32 i=0;i<index&&off+36<=len&&!zero32(b+off);i++){u32 n=*(u32*)(b+off+32);if(off+36+n>len){off=len>=32?len-32:0;break;}off+=36+n;}push(&off,4);cont();}
