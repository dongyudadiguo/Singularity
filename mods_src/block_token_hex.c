typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){static const char x[]="0123456789abcdef";u32 o=*(u32*)pop(4);char out[8]="<end>   ";if(o+32<=cvm_cached_len()){u8*p=cvm_cached_base()+o;int nz=0;for(int i=0;i<32;i++)nz|=p[i];if(nz)for(int i=0;i<4;i++){out[2*i]=x[p[i]>>4];out[2*i+1]=x[p[i]&15];}}push(out,8);cont();}
