#ifndef DXGFX_H
#define DXGFX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DXGFX_BUILD
#define DXGFX_API __declspec(dllexport)
#else
#define DXGFX_API __declspec(dllimport)
#endif

typedef unsigned char dx_u8;
typedef unsigned int dx_u32;

DXGFX_API int dxgfx_keyboard(dx_u8 out_state[256]);
DXGFX_API int dxgfx_mouse(int out_state[4]);
DXGFX_API int dxgfx_frame_begin(void);
DXGFX_API int dxgfx_clear(dx_u32 argb);
DXGFX_API int dxgfx_frame_end(void);
DXGFX_API int dxgfx_screen_size(int out_size[2]);
DXGFX_API int dxgfx_window_should_close(void);
DXGFX_API int dxgfx_input_snapshot(dx_u8 keys_down[256], dx_u8 keys_pressed[256], dx_u8 keys_released[256], int mouse[8], char text[64]);
DXGFX_API int dxgfx_set_camera(float target_x, float target_y, float zoom);
DXGFX_API int dxgfx_world_mouse(float out_xy[2]);
DXGFX_API int dxgfx_measure_text(float size, const char *utf8, dx_u32 len, float out_size[2]);
DXGFX_API int dxgfx_draw_text(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len);
DXGFX_API int dxgfx_draw_text_screen(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len);
DXGFX_API int dxgfx_mouse_f(float out_xy[2]);
DXGFX_API int dxgfx_mouse_wheel_f(float *out_notches);
DXGFX_API int dxgfx_draw_rect(float x, float y, float w, float h, dx_u32 argb, float stroke, int fill);
DXGFX_API int dxgfx_draw_line(float x1, float y1, float x2, float y2, dx_u32 argb, float stroke);
/* Draw icon named by token (loads icons/<name>.svg if present; else geometric fallback).
 * x,y are world top-left; size is world height/width of icon box. */
DXGFX_API int dxgfx_draw_icon(float x, float y, float size, dx_u32 argb, const char *name);
/* World-space measure of icon box (always square size). */
DXGFX_API float dxgfx_icon_size(float text_size);

#ifdef __cplusplus
}
#endif
#endif
