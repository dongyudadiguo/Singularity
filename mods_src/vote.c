#include "net_ops.h"
__declspec(dllexport) void run(void) {
    H parent, child, out;
    cvm_pop(child);
    cvm_pop(parent);
    cvm_zero(out);
    if (net_vote(parent, child)) out[0] = 1;
    cvm_push(out);
    cnext();
}
