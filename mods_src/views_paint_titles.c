#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) int cvm_key_dirty(const H key);
extern __declspec(dllimport) float cvm_heat_node(const H key);
extern __declspec(dllimport) float cvm_heat_uid(u32 uid);

static void draw_btn(float x, float y, float w, const char *label, u32 bg, u32 fg) {
    dxgfx_draw_rect(x, y, w, BTN_H, bg, 1.0f, 1);
    dxgfx_draw_text((int)(x + 6.0f), (int)(y + 1.0f), fg, 13.0f, label, (u32)strlen(label));
}

/* Network explorer (tag): show vote only, no latch/commit.
 * Block nodes: commit/latch/vote as before.
 * Heat white frame: cvm_heat_node(key) — cond_token pulses TARGET; cond_reexec pulses host.
 */
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
        int is_tag = key_is_tag(v->key);
        int dirty = (!is_tag) ? cvm_key_dirty(v->key) : 0;
        int latch = (v->pad0 & 1u) != 0;
        int show_rec = is_tag || !dirty; /* tags always can vote; blocks when clean */
        int show_latch = !is_tag;
        int show_commit = !is_tag && dirty && !latch;

        /* Heat white frame around this node when this key is hot */
        {
            float heat = cvm_heat_node(v->key);
            if (heat > 0.05f) {
                float rows = is_tag ? (float)tag_child_count(v->key) + 1.0f
                                    : (float)block_row_count(v) + 1.0f;
                float body_h = rows * 24.0f + 8.0f;
                float body_w = title_w + 120.0f;
                if (body_w < 120.0f) body_w = 120.0f;
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

        float bx = v->x + title_w + 10.0f;
        float by = v->y - 28.0f;
        if (show_commit) {
            float w = btn_w("commit");
            draw_btn(bx, by, w, "commit", 0xff3a6ea5, 0xffe8ecef);
            bx += w + BTN_GAP;
        }
        if (show_latch) {
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
