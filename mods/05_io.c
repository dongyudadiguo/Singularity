#include "cvm_mod.h"

static Host *G;

static int io_print(u8 *d, uint32_t n) {
    if (n) {
        fwrite(d, 1, n, stdout);
        fflush(stdout);
    } else {
        Buf *a = G->top();
        fwrite(a->p, 1, a->n, stdout);
        fflush(stdout);
    }
    return 0;
}

static int io_puts(u8 *d, uint32_t n) {
    io_print(d, n);
    putchar('\n');
    return 0;
}

static int io_readline(u8 *d, uint32_t n) {
    char s[4096];
    fgets(s, sizeof(s), stdin);
    G->push((u8 *)s, strlen(s));
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:IO:PRINT", io_print);
    h->op_name("CVM1:IO:PUTS", io_puts);
    h->op_name("CVM1:IO:READLINE", io_readline);
}
