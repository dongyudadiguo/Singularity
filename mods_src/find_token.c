#include "block_chain.h"
__declspec(dllexport) void run(void) {
    H chain_h, tok, out;
    u32 len = 0, off = 0, idx = 0;
    cvm_pop(tok);
    cvm_pop(chain_h);
    u8 *d = block_read(chain_h, &len);
    cvm_zero(out);
    if (d) {
        while (!bc_end(d, len, off)) {
            if (off + 36 > len) break;
            u32 sp = bc_span(d + off);
            if (sp < 4 || off + 32 + sp > len) break;
            if (memcmp(d + off, tok, 32) == 0) { cvm_u64_to_h(idx, out); break; }
            off += 32 + sp;
            idx++;
        }
        free(d);
    }
    cvm_push(out);
    cnext();
}
