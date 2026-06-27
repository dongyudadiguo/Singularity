#ifndef MOD_H
#define MOD_H

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_token(void);
extern __declspec(dllimport) void cvm_exec(const H h);
extern __declspec(dllimport) void cvm_exec_payload(H h);
extern __declspec(dllimport) void cvm_reexec(void);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

#endif
