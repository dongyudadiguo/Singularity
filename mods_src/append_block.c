#include "block_chain.h"
__declspec(dllexport) void run(void) {
    H chain_h, item_h, out;
    u32 clen = 0, ilen = 0;
    cvm_pop(item_h);
    cvm_pop(chain_h);
    u8 *chain = block_read(chain_h, &clen);
    u8 *item = block_read(item_h, &ilen);
    cvm_zero(out);
    if (chain && item) bc_save_edit(chain, clen, item, ilen, 0, 0, out);
    if (chain) free(chain);
    if (item) free(item);
    cvm_push(out);
    cnext();
}
