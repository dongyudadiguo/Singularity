typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
__declspec(dllexport) void run(void){
    u32 n = cvm_payload_size()>=4 ? *(u32*)cvm_payload() : 4;
    (void)from(n);
    cont();
}
