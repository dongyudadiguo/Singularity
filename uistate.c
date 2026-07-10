#define UISTATE_BUILD
#include "uistate.h"
#include <string.h>
static UiState state;
__declspec(dllexport) UiState *ui_state(void){ return &state; }
__declspec(dllexport) void ui_reset(void){ memset(&state,0,sizeof(state)); state.zoom=1.0f; state.dragging_view=-1; }
