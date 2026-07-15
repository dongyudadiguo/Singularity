#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) int cvm_has_dll(H h);
extern __declspec(dllimport) int cvm_cache_hit(const H k);
extern __declspec(dllimport) float cvm_heat_uid(u32 uid);

#define COL_DEFAULT   0xffe8ecef
#define COL_DLL       0xff5ec8e8
#define COL_OVERRIDE  0xffe0a050
#define COL_BOTH      0xffd080e0
#define COL_SUM       0xff7fb8d8
#define COL_VAR_ID    0xffc8e0a0
#define COL_VAR_SIZE  0xffe8c878
#define COL_END       0xff66717d
#define ICON_GAP 6.0f

static void fmt_id(const u8 *id, u32 n, char *out, u32 outn) {
    if (!n) { snprintf(out, outn, "<>"); return; }
    int printable = 1;
    for (u32 i = 0; i < n; i++) if (id[i] < 32 || id[i] > 126) { printable = 0; break; }
    if (printable) {
        u32 z = n < outn - 3 ? n : outn - 3;
        snprintf(out, outn, "%.*s", (int)z, (const char *)id);
        return;
    }
    u32 show = n < 12 ? n : 12;
    u32 pos = 0;
    for (u32 i = 0; i < show && pos + 2 < outn; i++)
        pos += (u32)snprintf(out + pos, outn - pos, "%02x", id[i]);
    if (n > show && pos + 2 < outn) snprintf(out + pos, outn - pos, "..");
}

static void row_summary(const u8 *tok, u32 tlen, const u8 *payload, u32 pn,
                        char *sum, u32 sumn, char *id_text, u32 idn, char *extra, u32 en, int *is_var) {
    sum[0]=id_text[0]=extra[0]=0; *is_var=0;
    const char *nm = (tlen == 32) ? token_name(tok) : 0;
    if (nm && (!strcmp(nm,"var_read_payload") || !strcmp(nm,"var_write_payload") || !strcmp(nm,"var_set_payload")
        || !strcmp(nm,"var_read") || !strcmp(nm,"var_write") || !strcmp(nm,"var_set"))) {
        *is_var = 1;
        if (!strcmp(nm,"var_read_payload")) { fmt_id(payload, pn, id_text, idn); return; }
        if (!strcmp(nm,"var_set") || !strcmp(nm,"var_read") || !strcmp(nm,"var_write")) {
            snprintf(id_text, idn, "%s", nm+4); return;
        }
        if (pn >= 4) {
            u32 id_len = *(u32*)payload;
            if (id_len > 0 && id_len <= 256 && pn >= 4 + id_len) {
                fmt_id(payload+4, id_len, id_text, idn);
                u32 rest = pn - 4 - id_len;
                if (!strcmp(nm,"var_set_payload") && rest == 4)
                    snprintf(extra, en, "%u", *(u32*)(payload+4+id_len));
                else if (rest) snprintf(extra, en, "%uB", rest);
                return;
            }
        }
        if (pn >= 32) {
            fmt_id(payload, 32, id_text, idn);
            if (pn == 36) snprintf(extra, en, "%u", *(u32*)(payload+32));
            else if (pn > 32) snprintf(extra, en, "%uB", pn-32);
            return;
        }
        return;
    }
    if (pn == 0) return;
    if (nm && !strcmp(nm, "cond_token_payload") && pn >= 32) {
        u32 uid = (pn >= 36) ? *(u32*)(payload+32) : 0;
        u8 once = (pn >= 38) ? payload[36] : 0;
        u8 contf = (pn >= 38) ? payload[37] : 0;
        snprintf(sum, sumn, "-> %s #%u%s%s", token_name(payload), uid,
                 contf ? " oo" : "", (!contf && once) ? " 1x" : "");
        return;
    }
    if (nm && !strcmp(nm, "cond_payload") && pn >= 32) {
        snprintf(sum, sumn, "-> %s", token_name(payload));
        return;
    }
    if (nm && !strcmp(nm, "cond_reexec")) {
        snprintf(sum, sumn, "reexec");
        return;
    }
    if (pn == 4) {
        u32 v = *(u32*)payload;
        if (v < 0x10000u) snprintf(sum, sumn, "%u", v);
        else snprintf(sum, sumn, "%g", (double)*(float*)payload);
        return;
    }
    u32 z = pn < 42 ? pn : 42;
    int printable = 1;
    for (u32 j = 0; j < z; j++) if (payload[j] < 32 || payload[j] > 126) { printable = 0; break; }
    if (printable) snprintf(sum, sumn, "'%.*s'", (int)z, (const char*)payload);
    else snprintf(sum, sumn, "[%u bytes]", pn);
}

static int g_gap_vi = -1;
static u32 g_gap_row = 0;

__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len); if (!t) { cont(); return; }

    float wmx = 0, wmy = 0;
    { float xy[2] = {0,0}; dxgfx_world_mouse(xy); wmx = xy[0]; wmy = xy[1]; }

    g_gap_vi = -1;
    g_gap_row = 0;
    float best_d = 1e9f;

    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used) continue;
        float dx = view_draw_x(t, vi);

        if (key_is_tag(v->key)) {
            u32 row = 0;
            float ry = v->y;
            u32 nch = tag_child_count(v->key);
            for (u32 r = 0; r < nch && r < 256; r++) {
                u8 child[32];
                if (!tag_child_at(v->key, r, child)) break;
                char nm[96];
                key_display_name(child, nm, sizeof(nm));
                int is_tag = key_is_tag(child);
                float name_w = measure_str(NAME_SIZE, nm);
                float mark_w = is_tag ? (NAME_GAP + measure_str(SUM_SIZE, "tag")) : 0.0f;
                float total_w = 4.0f + name_w + mark_w + PAD_X;
                if (total_w < MIN_HIT_W) total_w = MIN_HIT_W;
                int selected = (vi == t->active && row == v->cursor);
                if (selected) dxgfx_draw_rect(dx - 7.0f, ry - 2.0f, total_w, 23.0f, 0xff34414d, 1.0f, 1);
                u32 col = is_tag ? 0xffe0a050 : 0xffe8ecef;
                dxgfx_draw_text((int)dx, (int)ry, col, NAME_SIZE, nm, (u32)strlen(nm));
                if (is_tag) {
                    float cx = dx + name_w + NAME_GAP;
                    dxgfx_draw_text((int)cx, (int)ry, 0xff7fb8d8, SUM_SIZE, "tag", 3);
                }
                if (wmx >= dx - 20.0f && wmx <= dx + total_w + 40.0f) {
                    float d = wmy - ry; if (d < 0) d = -d;
                    if (d < best_d && d < 14.0f) { best_d = d; g_gap_vi = (int)vi; g_gap_row = row; }
                }
                ry += 24.0f;
                row++;
            }
            float end_w = measure_str(16.0f, "<end>") + 14.0f;
            if (end_w < 48.0f) end_w = 48.0f;
            int end_sel = (vi == t->active && row == v->cursor);
            if (end_sel) dxgfx_draw_rect(dx - 7.0f, ry - 2.0f, end_w, 23.0f, 0xff34414d, 1.0f, 1);
            dxgfx_draw_text((int)dx, (int)ry, COL_END, 16.0f, "<end>", 5);
            if (wmx >= dx - 20.0f && wmx <= dx + end_w + 40.0f) {
                float d = wmy - ry; if (d < 0) d = -d;
                if (d < best_d && d < 14.0f) { best_d = d; g_gap_vi = (int)vi; g_gap_row = row; }
            }
            continue;
        }

        H h; cvm_resolve_payload_hash(v->key, h);
        u8 *b = cvm_cached_base();
        u32 n = cvm_cached_len();
        u32 o = 0, row = 0;
        float ry = v->y;
        int mark_depth = 0;
        while (bl_ok(b, n, o) && !bl_is_end(b + o)) {
            u32 tlen = bl_tlen(b + o);
            const u8 *tok = bl_token_c(b + o);
            u32 pn = bl_plen(b + o);
            const u8 *payload = bl_payload_c(b + o);
            const char *nm = (tlen == 32) ? token_name(tok) : instr_token_name(b + o);

            int flags = 0;
            if (tlen == 32) {
                H th; memcpy(th, tok, 32);
                if (cvm_has_dll(th)) flags |= 1;
                if (cvm_cache_hit(th)) flags |= 2;
                { H vh; cvm_resolve_payload_hash(v->key, vh); b = cvm_cached_base(); n = cvm_cached_len(); }
            }

            u32 name_col = COL_DEFAULT;
            if (flags == 3) name_col = COL_BOTH;
            else if (flags == 1) name_col = COL_DLL;
            else if (flags == 2) name_col = COL_OVERRIDE;

            float heat = 0.0f;
            if (nm && !strcmp(nm, "cond_token_payload") && pn >= 36) {
                u32 uid = *(u32*)(payload + 32);
                heat = cvm_heat_uid(uid);
                if (heat > 0.05f) {
                    u32 a = (u32)(heat * 180.0f); if (a > 180) a = 180;
                    u32 bg = 0x002a5080u | (a << 24);
                    dxgfx_draw_rect(dx - 8.0f, ry - 2.0f, 220.0f, 23.0f, bg, 1.0f, 1);
                }
            }
            if (nm && !strcmp(nm, "cond_reexec")) {
                /* heat for reexec is node-local; painted on this row via node heat uid==0 special */
            }

            float icon_sz = dxgfx_icon_size(NAME_SIZE);
            float name_w = measure_str(NAME_SIZE, nm ? nm : "?");
            char id_text[96], extra[64], sum[100];
            int is_var = 0;
            row_summary(tok, tlen, payload, pn, sum, sizeof(sum), id_text, sizeof(id_text), extra, sizeof(extra), &is_var);
            const char *icon_name = nm;
            int has_icon = icon_name && dxgfx_has_icon(icon_name);
            float icon_w = has_icon ? (ICON_GAP + icon_sz) : 0.0f;
            float sum_w = 0.0f;
            if (is_var) {
                if (id_text[0]) sum_w += measure_str(SUM_SIZE, id_text);
                if (extra[0]) sum_w += NAME_GAP + measure_str(SUM_SIZE, extra);
            } else if (sum[0]) sum_w = measure_str(SUM_SIZE, sum);
            float gap = (sum_w > 0.0f) ? NAME_GAP : 0.0f;
            float total_w;
            if (is_var) total_w = 2.0f + (has_icon ? icon_sz + ICON_GAP : 0.0f) + sum_w + PAD_X;
            else total_w = 2.0f + name_w + icon_w + gap + sum_w + PAD_X;
            if (total_w < MIN_HIT_W) total_w = MIN_HIT_W;

            int selected = (vi == t->active && row == v->cursor);
            if (selected) dxgfx_draw_rect(dx - 7.0f + (float)mark_depth * 16.0f, ry - 2.0f, total_w, 23.0f, 0xff34414d, 1.0f, 1);

            /* mark/back nesting indent inside the instruction list */
            float row_indent = (float)mark_depth * 16.0f;
            if (nm && !strcmp(nm, "back") && mark_depth > 0) {
                /* draw back at parent level then decrease */
                row_indent = (float)(mark_depth - 1) * 16.0f;
            }
            float tx = dx + row_indent;
            if (nm && !strcmp(nm, "mark")) {
                /* gutter bar for new scope */
                dxgfx_draw_rect(tx - 8.0f, ry, 2.0f, 18.0f, 0xff62c982, 1.0f, 1);
            }
            if (is_var) {
                float cx = tx;
                if (has_icon) { dxgfx_draw_icon(cx, ry + 1.0f, icon_sz, name_col, icon_name); cx += icon_sz + ICON_GAP; }
                if (id_text[0]) { dxgfx_draw_text((int)cx, (int)ry, COL_VAR_ID, SUM_SIZE, id_text, (u32)strlen(id_text)); cx += measure_str(SUM_SIZE, id_text) + NAME_GAP; }
                if (extra[0]) dxgfx_draw_text((int)cx, (int)ry, COL_VAR_SIZE, SUM_SIZE, extra, (u32)strlen(extra));
            } else {
                dxgfx_draw_text((int)tx, (int)ry, name_col, NAME_SIZE, nm ? nm : "?", (u32)strlen(nm ? nm : "?"));
                float cx = tx + name_w;
                if (has_icon) { cx += ICON_GAP; dxgfx_draw_icon(cx, ry + 1.0f, icon_sz, name_col, icon_name); cx += icon_sz; }
                if (sum[0]) dxgfx_draw_text((int)(cx + NAME_GAP), (int)ry, COL_SUM, SUM_SIZE, sum, (u32)strlen(sum));
            }

            if (wmx >= dx - 20.0f && wmx <= dx + total_w + 80.0f) {
                float d = wmy - ry; if (d < 0) d = -d;
                if (d < best_d && d < 14.0f) { best_d = d; g_gap_vi = (int)vi; g_gap_row = row; }
            }

            if (nm && !strcmp(nm, "mark")) mark_depth++;
            else if (nm && !strcmp(nm, "back") && mark_depth > 0) mark_depth--;

            ry += 24.0f;
            o += bl_instr_size(b + o);
            row++;
            if (row > 256) break;
        }
        float end_w = measure_str(16.0f, "<end>") + 14.0f;
        if (end_w < 48.0f) end_w = 48.0f;
        int end_sel = (vi == t->active && row == v->cursor);
        if (end_sel) dxgfx_draw_rect(dx - 7.0f, ry - 2.0f, end_w, 23.0f, 0xff34414d, 1.0f, 1);
        dxgfx_draw_text((int)dx, (int)ry, COL_END, 16.0f, "<end>", 5);
        if (wmx >= dx - 20.0f && wmx <= dx + end_w + 80.0f) {
            float d = wmy - ry; if (d < 0) d = -d;
            if (d < best_d && d < 14.0f) { best_d = d; g_gap_vi = (int)vi; g_gap_row = row; }
        }
    }

    if (g_gap_vi >= 0 && (u32)g_gap_vi < t->count) {
        View *v = &t->views[g_gap_vi];
        float dx = view_draw_x(t, (u32)g_gap_vi);
        float gy = v->y + (float)g_gap_row * 24.0f;
        float gw = 80.0f;
        dxgfx_draw_rect(dx - 10.0f, gy - 1.0f, gw, 2.0f, 0xff62c982, 1.0f, 1);
        dxgfx_draw_rect(dx - 10.0f, gy - 5.0f, 3.0f, 10.0f, 0xff62c982, 1.0f, 1);
        /* Soft insert focus only when mouse buttons up — never fight LMB select. */
        int mstate[4] = {0,0,0,0};
        dxgfx_mouse(mstate);
        int buttons = mstate[2];
        if ((buttons & 3) == 0) {
            if (t->active != (u32)g_gap_vi || v->cursor != g_gap_row) {
                Table tt = *t;
                tt.active = (u32)g_gap_vi;
                tt.views[g_gap_vi].cursor = g_gap_row;
                store_table(id, id_len, &tt);
            }
        }
    }
    cont();
}
