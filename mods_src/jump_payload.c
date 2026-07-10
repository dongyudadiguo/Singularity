typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H,H); extern __declspec(dllimport) u8*cvm_cached_base(void);
extern __declspec(dllimport) void cvm_replace_current(const H,u8*); extern __declspec(dllimport) void cvm_exec(const H);
__declspec(dllexport) void run(void){H key,hash,first;if(cvm_payload_size()<32)return;for(int i=0;i<32;i++)key[i]=cvm_payload()[i];cvm_resolve_payload_hash(key,hash);cvm_replace_current(key,cvm_cached_base());for(int i=0;i<32;i++)first[i]=cvm_cached_base()[i];cvm_exec(first);}
