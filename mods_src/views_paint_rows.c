#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) int cvm_has_dll(H h);
extern __declspec(dllimport) int cvm_cache_hit(const H k);

#define COL_DEFAULT   0xffe8ecef
#define COL_DLL       0xff5ec8e8   /* cyan: native DLL present */
#define COL_OVERRIDE  0xffe0a050   /* amber: content in local block cache */
#define COL_BOTH      0xffd080e0   /* magenta: DLL + cache */
#define COL_SW_DLL    0xff3aa0c8
#define COL_SW_OVR    0xffc88830
#define COL_SW_BOTH   0xffb060c0
#define COL_SUM       0xff7fb8d8
#define COL_VAR_ID    0xffc8e0a0
#define COL_VAR_SIZE  0xffe8c878
#define COL_END       0xff66717d
#define COL_SW_NONE   0xff3a424a
#define ICON_GAP 6.0f
#define SWATCH_W 4.0f

static int same32(const u8 *a, const u8 *b) { return !memcmp(a, b, 32); }

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

/* Minimal row summary: no specialized registry of token types beyond name. */
static void row_summary(const u8 *tok, const u8 *payload, u32 pn, char *sum, u32 sumn,
                        char *id_text, u32 idn, char *extra, u32 en, int *is_var) {
    sum[0]=id_text[0]=extra[0]=0; *is_var=0;
    const char *nm = token_name(tok);
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
    if (nm && !strcmp(nm, "cond_payload") && pn >= 32) {
        snprintf(sum, sumn, "-> %s", token_name(payload));
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

/* payload: views_var[32] — draw instruction rows + <end> for each view */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; if (!payload_id(&id, &id_len, 0, 0)) { cont(); return; }
    Table *t = load_table(id, id_len); if (!t) { cont(); return; }
    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used) continue;
        H h; cvm_resolve_payload_hash(v->key, h);
        u8 *b = cvm_cached_base();
        u32 n = cvm_cached_len();
        u32 o = 0, row = 0;
        float ry = v->y;
        while (o + 36 <= n && !zero_key(b + o)) {
            u32 pn = *(u32*)(b + o + 32);
            if (o + 36 + pn > n) break;
            const u8 *tok = b + o;
            const u8 *payload = b + o + 36;
            const char *nm = token_name(tok);
            /* Local-only (no network): bit0=DLL, bit1=block-cache key hit.
             * cvm_cache_hit mutates primary_idx — restore view stream after. */
            int flags = 0;
            H th; memcpy(th, tok, 32);
            if (cvm_has_dll(th)) flags |= 1;
            if (cvm_cache_hit(th)) flags |= 2;
            /* restore this view's cached instruction block as primary */
            { H vh; cvm_resolve_payload_hash(v->key, vh); b = cvm_cached_base(); n = cvm_cached_len(); }
            u32 name_col = COL_DEFAULT;
            u32 sw_col = COL_SW_NONE;
            if (flags == 3) { name_col = COL_BOTH; sw_col = COL_SW_BOTH; }
            else if (flags == 1) { name_col = COL_DLL; sw_col = COL_SW_DLL; }
            else if (flags == 2) { name_col = COL_OVERRIDE; sw_col = COL_SW_OVR; }
            float icon_sz = dxgfx_icon_size(NAME_SIZE);
            float name_w = measure_str(NAME_SIZE, nm);
            char id_text[96], extra[64], sum[100];
            int is_var = 0;
            row_summary(tok, payload, pn, sum, sizeof(sum), id_text, sizeof(id_text), extra, sizeof(extra), &is_var);
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
            if (is_var) total_w = SWATCH_W + 4.0f + (has_icon ? icon_sz + ICON_GAP : 0.0f) + sum_w + PAD_X;
            else total_w = SWATCH_W + 4.0f + name_w + icon_w + gap + sum_w + PAD_X;
            if (total_w < MIN_HIT_W) total_w = MIN_HIT_W;
            int selected = (vi == t->active && row == v->cursor);
            if (selected) dxgfx_draw_rect(v->x - 7.0f, ry - 2.0f, total_w, 23.0f, 0xff34414d, 1.0f, 1);
            dxgfx_draw_rect(v->x - 6.0f, ry + 2.0f, SWATCH_W, 14.0f, sw_col, 1.0f, 1);
            float tx = v->x + SWATCH_W + 2.0f;
            if (is_var) {
                float cx = tx;
                if (has_icon) { dxgfx_draw_icon(cx, ry + 1.0f, icon_sz, name_col, icon_name); cx += icon_sz + ICON_GAP; }
                if (id_text[0]) { dxgfx_draw_text((int)cx, (int)ry, COL_VAR_ID, SUM_SIZE, id_text, (u32)strlen(id_text)); cx += measure_str(SUM_SIZE, id_text) + NAME_GAP; }
                if (extra[0]) dxgfx_draw_text((int)cx, (int)ry, COL_VAR_SIZE, SUM_SIZE, extra, (u32)strlen(extra));
            } else {
                dxgfx_draw_text((int)tx, (int)ry, name_col, NAME_SIZE, nm, (u32)strlen(nm));
                float cx = tx + name_w;
                if (has_icon) { cx += ICON_GAP; dxgfx_draw_icon(cx, ry + 1.0f, icon_sz, name_col, icon_name); cx += icon_sz; }
                if (sum[0]) dxgfx_draw_text((int)(cx + NAME_GAP), (int)ry, COL_SUM, SUM_SIZE, sum, (u32)strlen(sum));
            }
            ry += 24.0f;
            o += 36 + pn;
            row++;
            if (row > 256) break;
        }
        float end_w = measure_str(16.0f, "<end>") + 14.0f;
        if (end_w < 48.0f) end_w = 48.0f;
        int end_sel = (vi == t->active && row == v->cursor);
        if (end_sel) dxgfx_draw_rect(v->x - 7.0f, ry - 2.0f, end_w, 23.0f, 0xff34414d, 1.0f, 1);
        dxgfx_draw_text((int)v->x, (int)ry, COL_END, 16.0f, "<end>", 5);
    }
    cont();
}
