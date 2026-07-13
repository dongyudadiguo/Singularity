#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) int cvm_key_dirty(const H key);
extern __declspec(dllimport) float cvm_heat_node(const H key);
extern __declspec(dllimport) float cvm_heat_uid(u32 uid);

static void draw_btn(float x, float y, float w, const char *label, u32 bg, u32 fg) {
    dxgfx_draw_rect(x, y, w, BTN_H, bg, 1.0f, 1);
    dxgfx_draw_text((int)(x + 6.0f), (int)(y + 1.0f), fg, 13.0f, label, (u32)strlen(label));
}

/* payload: views_var — draw titles + commit/latch/vote chrome + heat white frame */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len); if (!t) { cont(); return; }
    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used) continue;
        char dname[80];
        key_display_name(v->key, dname, sizeof(dname));
        char title[120];
        snprintf(title, sizeof(title), "[%u] %s", vi, dname);
        float title_w = measure_str(16.0f, title);
        int dirty = 0;
        if (!key_is_tag(v->key)) dirty = cvm_key_dirty(v->key);
        int latch = (v->pad0 & 1u) != 0;
        int show_rec = !dirty && !key_is_tag(v->key);

        /* Heat white frame: max cond_token uid heat in this block (or node map). */
        {
            float heat = cvm_heat_node(v->key);
            if (!key_is_tag(v->key)) {
                H hh; cvm_resolve_payload_hash(v->key, hh);
                u8 *b = cvm_cached_base();
                u32 n = cvm_cached_len();
                u32 o = 0;
                while (o + 36 <= n && !zero_key(b + o)) {
                    u32 pn = *(u32*)(b + o + 32);
                    if (o + 36 + pn > n) break;
                    const char *nm = token_name(b + o);
                    if (nm && !strcmp(nm, "cond_token_payload") && pn >= 36) {
                        u32 uid = *(u32*)(b + o + 36 + 32);
                        float h2 = cvm_heat_uid(uid);
                        if (h2 > heat) heat = h2;
                    }
                    o += 36 + pn;
                }
            }
            if (heat > 0.05f) {
                float rows = 8.0f;
                if (key_is_tag(v->key)) rows = (float)tag_child_count(v->key) + 1.0f;
                else rows = (float)block_row_count(v) + 1.0f;
                float body_h = rows * 24.0f + 8.0f;
                float body_w = header_total_width(vi, v, dirty, show_rec);
                if (body_w < 120.0f) body_w = 120.0f;
                /* alpha-ish via brightness on white border */
                u32 a = (u32)(heat * 255.0f); if (a > 255) a = 255;
                u32 col = 0x00ffffffu | (a << 24);
                dxgfx_draw_rect(v->x - 10.0f, v->y - 34.0f, body_w + 16.0f, body_h + 36.0f, col, 2.0f, 0);
            }
        }

        if (vi == t->active) {
            float tw = title_w + 16.0f;
            if (tw < 72.0f) tw = 72.0f;
            dxgfx_draw_rect(v->x - 6.0f, v->y - 30.0f, tw, 22.0f, 0xff2a333c, 1.0f, 1);
        }
        dxgfx_draw_text((int)v->x, (int)(v->y - 28.0f), 0xff9da7b3, 16.0f, title, (u32)strlen(title));

        /* Buttons to the right of title */
        float bx = v->x + title_w + 10.0f;
        float by = v->y - 28.0f;
        if (dirty && !latch) {
            float w = btn_w("commit");
            draw_btn(bx, by, w, "commit", 0xff3a6ea5, 0xffe8ecef);
            bx += w + BTN_GAP;
        }
        {
            const char *lb = latch ? "latch*" : "latch";
            float w = btn_w(lb);
            u32 bg = latch ? 0xff2f6f4e : 0xff3a424a;
            draw_btn(bx, by, w, lb, bg, 0xffe8ecef);
            bx += w + BTN_GAP;
        }
        if (show_rec) {
            float w = btn_w("vote");
            draw_btn(bx, by, w, "vote", 0xff6b4f9a, 0xffe8ecef);
        }
    }
    cont();
}
