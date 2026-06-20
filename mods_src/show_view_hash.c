#include "io_parse.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); printf("view: "); if(s)print_h(s->view_hash); printf("\n"); cnext(); }
