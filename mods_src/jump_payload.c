typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8*cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H,H);
extern __declspec(dllimport) u8*cvm_cached_base(void);
extern __declspec(dllimport) void cvm_replace_current(const H,u8*);
extern __declspec(dllimport) void cvm_exec_instr(void);
/* Jump to program key: replace stream and dispatch. Always ends via exec or cont. */
__declspec(dllexport) void run(void){
    H key, hash;
    if (cvm_payload_size() < 32) { cont(); return; }
    for (int i = 0; i < 32; i++) key[i] = cvm_payload()[i];
    cvm_resolve_payload_hash(key, hash);
    cvm_replace_current(key, cvm_cached_base());
    cvm_exec_instr();
}
