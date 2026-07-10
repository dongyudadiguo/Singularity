typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8*cvm_var_get(const H,u32*); extern __declspec(dllimport) void cvm_var_write(const H,const u8*,u32);
__declspec(dllexport) void run(void){if(cvm_payload_size()>=32){u32 n;u8*s=cvm_var_get(cvm_payload(),&n);if(s&&n){s[0]=0;cvm_var_write(cvm_payload(),s,n);}}cont();}
