#include "../cvm_state.h"
#include "../continue.h"

__declspec(dllexport) void run(void) {
    H dividend_h, divisor_h, out;
    cvm_pop(divisor_h);
    cvm_pop(dividend_h);
    u64 divisor = cvm_h_to_u64(divisor_h);
    cvm_u64_to_h(divisor ? (cvm_h_to_u64(dividend_h) % divisor) : 0, out);
    cvm_push(out);
    cnext();
}
