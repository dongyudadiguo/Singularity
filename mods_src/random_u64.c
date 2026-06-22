#include "../cvm_state.h"
#include "../continue.h"

__declspec(dllexport) void run(void) {
    H out;
    u64 v = 0;
    HCRYPTPROV hp = 0;
    if (CryptAcquireContextW(&hp, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hp, sizeof(v), (BYTE*)&v);
        CryptReleaseContext(hp, 0);
    } else {
        v = ((u64)GetTickCount64() << 16) ^ (u64)(UINT_PTR)&v;
    }
    cvm_u64_to_h(v, out);
    cvm_push(out);
    cnext();
}
