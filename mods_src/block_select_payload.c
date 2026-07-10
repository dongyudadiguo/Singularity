typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H,H);
__declspec(dllexport) void run(void){H h;if(cvm_payload_size()>=32)cvm_resolve_payload_hash(cvm_payload(),h);cont();}
