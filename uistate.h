#ifndef UISTATE_H
#define UISTATE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef UISTATE_BUILD
#define UI_API __declspec(dllexport)
#else
#define UI_API __declspec(dllimport)
#endif
typedef unsigned char ui_u8;
typedef unsigned ui_u32;
typedef ui_u8 ui_hash[32];
#define UI_MAX_REG 2048
#define UI_MAX_VIEWS 32
#define UI_MAX_COPY (1u << 18)
typedef struct { ui_hash token; char name[96]; int tag; } UiReg;
typedef struct { ui_hash key; float x,y; int used, linked, parent; float link_x,link_y; } UiView;
typedef struct {
 int ready, registry_ready, dirty;
 ui_hash program_key, registry_key;
 UiReg reg[UI_MAX_REG]; ui_u32 reg_count;
 UiView views[UI_MAX_VIEWS]; ui_u32 view_count, active_view;
 ui_u32 point_off, mark_off; int marking;
 float cam_x,cam_y,zoom,last_mx,last_my;
 char input[256],completion[96]; ui_u32 completion_index;
 ui_u8 copy[UI_MAX_COPY]; ui_u32 copy_len;
 int dragging_view;
 ui_u8 keys_down[256], keys_pressed[256], keys_released[256];
 int mouse[8]; char text[64]; float world_mouse[2];
} UiState;
UI_API UiState *ui_state(void);
UI_API void ui_reset(void);
#ifdef __cplusplus
}
#endif
#endif
