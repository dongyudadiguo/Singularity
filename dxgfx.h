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
DXGFX_API int dxgfx_draw_text(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len);
DXGFX_API int dxgfx_draw_rect(float x, float y, float w, float h, dx_u32 argb, float stroke, int fill);
DXGFX_API int dxgfx_draw_line(float x1, float y1, float x2, float y2, dx_u32 argb, float stroke);

#ifdef __cplusplus
}
#endif
#endif
