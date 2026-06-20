#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H key, val, out;
    cvm_pop(val);
    cvm_pop(key);
    cvm_zero(out);
    if (net_uset(key, val)) out[0] = 1;
    cvm_push(out);
    cnext();
}
