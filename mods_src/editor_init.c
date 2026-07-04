extern __declspec(dllimport) void cont(void);
#include "../editorcore.h"
__declspec(dllexport) void run(void){ ec_init(); cont(); }
