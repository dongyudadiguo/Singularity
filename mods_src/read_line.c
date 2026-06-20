#include "io_parse.h"
__declspec(dllexport) void run(void) {
    char buf[4096];
    H out;
    cvm_zero(out);
    if (fgets(buf, sizeof(buf), stdin)) {
        u32 len = (u32)strlen(buf);
        while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) len--;
        block_write((u8*)buf, len, out);
    }
    cvm_push(out);
    cnext();
}
