#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) float cvm_heat_uid(u32 uid);

/* Toggle once/continuous bits in parent block for linked child.
 * Stored in process-local for hit tests this frame. */
typedef struct {
    int on;
    float x, y, w, h;
    u32 parent_vi;
    u32 parent_row;
    int which; /* 1=once 2=continuous */
} LinkBtn;
static LinkBtn g_link_btns[64];
static u32 g_link_btn_n;

static void add_btn(float x, float y, float w, float h, u32 pvi, u32 prow, int which) {
    if (g_link_btn_n >= 64) return;
    LinkBtn *b = &g_link_btns[g_link_btn_n++];
    b->on = 1; b->x = x; b->y = y; b->w = w; b->h = h;
    b->parent_vi = pvi; b->parent_row = prow; b->which = which;
}

/* Exported for apply_lmb via shared static — same DLL only if same TU.
 * So we use a small C file approach: store buttons in a var instead.
 * Simpler: put hit test logic in apply_lmb by recomputing midpoints.
 * Here we only paint.
 */

static int row_from_link_y(float link_y, float row_h) {
    int r = (int)((link_y - 10.0f) / row_h + 0.5f);
    if (r < 0) r = 0;
    return r;
}

/* payload: views_var — parent-child links + cond once/∞ mid controls + heat */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len); if (!t) { cont(); return; }
    g_link_btn_n = 0;
    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used || !v->linked || v->parent < 0) continue;
        if ((u32)v->parent >= t->count || !t->views[v->parent].used) continue;
        View *p = &t->views[v->parent];
        float x1 = p->x + v->link_x;
        float y1 = p->y + v->link_y;
        float x2 = v->x;
        float y2 = v->y;
        float mx = (x1 + x2) * 0.5f;
        float my = (y1 + y2) * 0.5f;

        /* Default green; heat brightens toward white/cyan */
        u32 col = 0xff62c982;
        float heat = 0.0f;
        u8 once = 0, contf = 0;
        int is_cond = 0;
        u32 prow = (u32)row_from_link_y(v->link_y, 24.0f);

        if (!key_is_tag(p->key)) {
            const u8 *instr = row_instr(p, prow, 0);
            if (instr) {
                const char *nm = token_name(instr);
                u32 pn = *(u32*)(instr + 32);
                const u8 *pay = instr + 36;
                if (nm && !strcmp(nm, "cond_token_payload") && pn >= 32) {
                    is_cond = 1;
                    u32 uid = 0;
                    cond_token_parse(pay, pn, 0, &uid, &once, &contf);
                    heat = cvm_heat_uid(uid);
                } else if (nm && (!strcmp(nm, "cond_payload") || !strcmp(nm, "jump_payload") || !strcmp(nm, "exec_payload"))) {
                    is_cond = 0;
                }
            }
        }
        if (heat > 0.05f) {
            u32 a = (u32)(180 + heat * 75.0f); if (a > 255) a = 255;
            col = 0x0000e0ffu | (a << 24); /* bright cyan-ish */
            /* blend: use opaque warm */
            col = 0xffa0f0ff;
        }
        dxgfx_draw_line(x1, y1, x2, y2, col, heat > 0.05f ? 3.0f : 2.0f);

        if (is_cond) {
            /* mid controls: [1x] [∞]; hide 1x if continuous */
            float bx = mx - 28.0f;
            float by = my - 9.0f;
            if (!contf) {
                u32 bg = once ? 0xff3a6ea5 : 0xff3a424a;
                dxgfx_draw_rect(bx, by, 28.0f, 18.0f, bg, 1.0f, 1);
                dxgfx_draw_text((int)(bx + 6), (int)(by + 1), 0xffe8ecef, 12.0f, "1x", 2);
                add_btn(bx, by, 28.0f, 18.0f, (u32)v->parent, prow, 1);
                bx += 32.0f;
            }
            {
                u32 bg = contf ? 0xff2f6f4e : 0xff3a424a;
                dxgfx_draw_rect(bx, by, 28.0f, 18.0f, bg, 1.0f, 1);
                dxgfx_draw_text((int)(bx + 8), (int)(by + 1), 0xffe8ecef, 12.0f, "oo", 2);
                add_btn(bx, by, 28.0f, 18.0f, (u32)v->parent, prow, 2);
            }
        }
    }
    cont();
}

/* Hit-test mid-link buttons; returns which (1/2) and fills parent_vi/row. 0 if miss.
 * Note: only valid after paint_links in same process if we expose g_link_btns.
 * Since each mod is its own DLL, static is NOT shared. apply_lmb recomputes.
 */
