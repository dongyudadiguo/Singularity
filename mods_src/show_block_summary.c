#include "block_chain.h"
#include "io_parse.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (s) {
        u32 len=0, off=0, idx=0;
        u8 *d=block_read(s->view_hash,&len);
        printf("blocks: %u\n", d ? bc_count(d,len) : 0);
        while (d && !bc_end(d,len,off)) {
            if (off+36>len) break;
            u32 sp=bc_span(d+off);
            printf("%u token=",idx); H h; memcpy(h,d+off,32); print_h(h); printf(" span=%u\n",sp);
            if (sp<4 || off+32+sp>len) break;
            off += 32 + sp; idx++;
        }
        if(d)free(d);
    }
    cnext();
}
