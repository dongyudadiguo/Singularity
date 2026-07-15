#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) float cvm_heat_uid(u32 uid);
extern __declspec(dllimport) float cvm_heat_node(const H key);
extern __declspec(dllimport) int cvm_hand_armed(u32 uid);

static int row_from_link_y(float link_y, float row_h) {
    int r = (int)((link_y - 10.0f) / row_h + 0.5f);
    if (r < 0) r = 0;
    return r;
}

/* Draw parent-child links for block explorer only (NOT tag/network nodes).
 * token_run_by_hand: mid-link arm toggle button + heat.
 * cond_token_payload: heat only (no once/continuous chrome).
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len); if (!t) { cont(); return; }
    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used || !v->linked || v->parent < 0) continue;
        if ((u32)v->parent >= t->count || !t->views[v->parent].used) continue;
        View *p = &t->views[v->parent];
        /* Network/tag explorer: no link lines */
        if (key_is_tag(p->key) || key_is_tag(v->key)) continue;

        float x1 = view_draw_x(t, (u32)v->parent) + v->link_x;
        float y1 = p->y + v->link_y;
        float x2 = view_draw_x(t, vi);
        float y2 = v->y;
        float mx = (x1 + x2) * 0.5f;
        float my = (y1 + y2) * 0.5f;

        u32 col = 0xff62c982;
        float heat = cvm_heat_node(v->key);
        int is_hand = 0;
        u32 hand_uid = 0;
        u32 prow = (u32)row_from_link_y(v->link_y, 24.0f);

        if (!key_is_tag(p->key)) {
            const u8 *instr = row_instr(p, prow, 0);
            if (instr && bl_tlen(instr) == 32) {
                const char *nm = token_name(bl_token_c(instr));
                u32 pn = bl_plen(instr);
                const u8 *pay = bl_payload_c(instr);
                if (nm && !strcmp(nm, "cond_token_payload") && pn >= 36) {
                    u32 uid = *(u32*)(pay + 32);
                    float h2 = cvm_heat_uid(uid);
                    if (h2 > heat) heat = h2;
                }
                if (nm && !strcmp(nm, "token_run_by_hand") && pn >= 36) {
                    is_hand = 1;
                    hand_uid = *(u32*)pay;
                    float h2 = cvm_heat_uid(hand_uid);
                    if (h2 > heat) heat = h2;
                }
                if (nm && !strcmp(nm, "cond_reexec") && pn >= 4) {
                    u32 uid = *(u32*)pay;
                    float h2 = cvm_heat_uid(uid);
                    if (h2 > heat) heat = h2;
                }
            }
        }
        if (heat > 0.05f) col = 0xffa0f0ff;
        dxgfx_draw_line(x1, y1, x2, y2, col, heat > 0.05f ? 3.0f : 2.0f);

        if (is_hand) {
            int armed = cvm_hand_armed(hand_uid);
            float bw = 36.0f, bh = 18.0f;
            float bx = mx - bw * 0.5f, by = my - bh * 0.5f;
            u32 bg = armed ? 0xff2f6f4e : 0xff3a424a;
            dxgfx_draw_rect(bx, by, bw, bh, bg, 1.0f, 1);
            const char *lb = armed ? "ON" : "off";
            dxgfx_draw_text((int)(bx + 6), (int)(by + 1), 0xffe8ecef, 12.0f, lb, (u32)strlen(lb));
        }
    }
    cont();
}
