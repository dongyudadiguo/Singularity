#ifndef MOD_SURFACE_OPS_H
#define MOD_SURFACE_OPS_H

#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"

#define CVM_SURFACE_CONTEXT_MAGIC 0x53564332u
#define CVM_SURFACE_STACK_CAP 64

typedef struct {
    LONG tx;
    LONG ty;
} CvmSurfaceTranslateFrame;

typedef struct {
    int active;
    RECT clip;
} CvmSurfaceClipFrame;

typedef struct {
    u32 magic;
    LONG tx;
    LONG ty;
    int clip_active;
    RECT clip;
    CvmSurfaceTranslateFrame translate_stack[CVM_SURFACE_STACK_CAP];
    u32 translate_sp;
    CvmSurfaceClipFrame clip_stack[CVM_SURFACE_STACK_CAP];
    u32 clip_sp;
    u32 last_char;
    u32 pending_high_surrogate;
} CvmSurfaceContext;

static CvmSurfaceContext* surface_context(void) {
    static CvmSurfaceContext *ctx;
    static HANDLE map;
    if (ctx) return ctx;
    map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, "Local\\CVM_Surface_Context");
    if (!map) map = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(CvmSurfaceContext), "Local\\CVM_Surface_Context");
    if (!map) return 0;
    ctx = (CvmSurfaceContext*)MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CvmSurfaceContext));
    if (ctx && ctx->magic != CVM_SURFACE_CONTEXT_MAGIC) {
        memset(ctx, 0, sizeof(*ctx));
        ctx->magic = CVM_SURFACE_CONTEXT_MAGIC;
    }
    return ctx;
}

static void surface_context_reset(void) {
    CvmSurfaceContext *ctx = surface_context();
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->magic = CVM_SURFACE_CONTEXT_MAGIC;
}

static LONG surface_h_to_long(const H h) {
    return (LONG)cvm_h_to_u64(h);
}

static LRESULT CALLBACK surface_proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    CvmState *s = cvm_state();
    CvmSurfaceContext *ctx = surface_context();
    if (s) {
        if (m == WM_LBUTTONDOWN || m == WM_MOUSEMOVE || m == WM_LBUTTONUP) {
            s->surface_event = m;
            s->surface_x = (u64)(short)LOWORD(lp);
            s->surface_y = (u64)(short)HIWORD(lp);
            if (ctx) ctx->last_char = 0;
        } else if (m == WM_CHAR) {
            s->surface_event = m;
            if (ctx) {
                u32 ch = (u32)wp;
                if (ch >= 0xd800 && ch <= 0xdbff) {
                    ctx->pending_high_surrogate = ch;
                    ctx->last_char = 0;
                } else if (ch >= 0xdc00 && ch <= 0xdfff && ctx->pending_high_surrogate) {
                    ctx->last_char = 0x10000 + (((ctx->pending_high_surrogate - 0xd800) << 10) | (ch - 0xdc00));
                    ctx->pending_high_surrogate = 0;
                } else {
                    ctx->last_char = ch;
                    ctx->pending_high_surrogate = 0;
                }
            }
        } else if (m == WM_KEYDOWN) {
            s->surface_event = 0x10000 | (u64)wp;
            if (ctx) ctx->last_char = 0;
        } else if (m == WM_SIZE) {
            if (!s->surface_event) s->surface_event = m;
            s->surface_w = (u64)LOWORD(lp);
            s->surface_h = (u64)HIWORD(lp);
        } else if (m == WM_PAINT) {
            PAINTSTRUCT ps;
            BeginPaint(w, &ps);
            EndPaint(w, &ps);
            if (!s->surface_event) s->surface_event = m;
            if (ctx) ctx->last_char = 0;
            return 0;
        } else if (m == WM_ERASEBKGND) {
            return 1;
        } else if (m == WM_CLOSE) {
            s->surface_event = 0xffffffff;
            if (ctx) ctx->last_char = 0;
            ShowWindow(w, SW_HIDE);
            return 0;
        }
    }
    return DefWindowProcW(w, m, wp, lp);
}

static void surface_class(void) {
    static int done;
    if (done) return;
    done = 1;
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = surface_proc;
    wc.hInstance = GetModuleHandleW(0);
    wc.lpszClassName = L"CVM_Surface";
    wc.hCursor = LoadCursorW(0, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = 0;
    RegisterClassW(&wc);
}

static HWND surface_hwnd(void) {
    CvmState *s = cvm_state();
    if (!s || !s->surface_hwnd) return 0;
    if (!IsWindow(s->surface_hwnd)) {
        s->surface_hwnd = 0;
        return 0;
    }
    return s->surface_hwnd;
}

static void surface_rect_from_h(H h, RECT *r) {
    H v;
    CvmSurfaceContext *ctx = surface_context();
    LONG tx = ctx ? ctx->tx : 0;
    LONG ty = ctx ? ctx->ty : 0;
    cvm_zero(v); memcpy(v, h, 8); r->left = (LONG)cvm_h_to_u64(v) + tx;
    cvm_zero(v); memcpy(v, h + 8, 8); r->top = (LONG)cvm_h_to_u64(v) + ty;
    cvm_zero(v); memcpy(v, h + 16, 8); r->right = r->left + (LONG)cvm_h_to_u64(v);
    cvm_zero(v); memcpy(v, h + 24, 8); r->bottom = r->top + (LONG)cvm_h_to_u64(v);
}

static void surface_apply_clip(HDC dc) {
    CvmSurfaceContext *ctx = surface_context();
    if (dc && ctx && ctx->clip_active) {
        IntersectClipRect(dc, ctx->clip.left, ctx->clip.top, ctx->clip.right, ctx->clip.bottom);
    }
}

static LONG surface_coord_x(H h) {
    CvmSurfaceContext *ctx = surface_context();
    return surface_h_to_long(h) + (ctx ? ctx->tx : 0);
}

static LONG surface_coord_y(H h) {
    CvmSurfaceContext *ctx = surface_context();
    return surface_h_to_long(h) + (ctx ? ctx->ty : 0);
}

static void surface_clip_push_rect(H h) {
    CvmSurfaceContext *ctx = surface_context();
    if (!ctx || ctx->clip_sp >= CVM_SURFACE_STACK_CAP) return;
    ctx->clip_stack[ctx->clip_sp].active = ctx->clip_active;
    ctx->clip_stack[ctx->clip_sp].clip = ctx->clip;
    ctx->clip_sp++;

    RECT next;
    surface_rect_from_h(h, &next);
    RECT base;
    HWND w = surface_hwnd();
    if (ctx->clip_active) {
        base = ctx->clip;
    } else if (w) {
        GetClientRect(w, &base);
    } else {
        base = next;
    }
    RECT out;
    if (!IntersectRect(&out, &base, &next)) SetRectEmpty(&out);
    ctx->clip = out;
    ctx->clip_active = 1;
}

static void surface_clip_pop_rect(void) {
    CvmSurfaceContext *ctx = surface_context();
    if (!ctx || !ctx->clip_sp) return;
    ctx->clip_sp--;
    ctx->clip_active = ctx->clip_stack[ctx->clip_sp].active;
    ctx->clip = ctx->clip_stack[ctx->clip_sp].clip;
}

static void surface_translate_push_xy(LONG x, LONG y) {
    CvmSurfaceContext *ctx = surface_context();
    if (!ctx || ctx->translate_sp >= CVM_SURFACE_STACK_CAP) return;
    ctx->translate_stack[ctx->translate_sp].tx = ctx->tx;
    ctx->translate_stack[ctx->translate_sp].ty = ctx->ty;
    ctx->translate_sp++;
    ctx->tx += x;
    ctx->ty += y;
}

static void surface_translate_pop_xy(void) {
    CvmSurfaceContext *ctx = surface_context();
    if (!ctx || !ctx->translate_sp) return;
    ctx->translate_sp--;
    ctx->tx = ctx->translate_stack[ctx->translate_sp].tx;
    ctx->ty = ctx->translate_stack[ctx->translate_sp].ty;
}

#endif
