#include "../cvm_state.h"
#include "../block.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H target;
    u32 len;
    cvm_pop(target);
    u8 *p = block(&target, &len);
    if (!p) return;
    while (cont(&p)) {
        CvmState *s = cvm_state();
        if (s && s->ret) break;
    }
}
