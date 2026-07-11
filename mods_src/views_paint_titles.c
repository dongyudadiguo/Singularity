#include "views_common.h"
#include <stdlib.h>

#define COL_DEFAULT   0xffe8ecef
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

/* payload: views_var[32] — draw view titles / active highlight */
__declspec(dllexport) void run(void){
    H id; if (!payload_id(id,0,0)) { cont(); return; }
    Table *t = load_table(id); if (!t) { cont(); return; }
    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used) continue;
        char title[80];
        snprintf(title, sizeof(title), "[%u] %02x%02x%02x%02x",
                 vi, v->key[0], v->key[1], v->key[2], v->key[3]);
        float title_w = measure_str(16.0f, title);
        if (vi == t->active) {
            float tw = title_w + 16.0f;
            if (tw < 72.0f) tw = 72.0f;
            dxgfx_draw_rect(v->x - 6.0f, v->y - 30.0f, tw, 22.0f, 0xff2a333c, 1.0f, 1);
        }
        dxgfx_draw_text((int)v->x, (int)(v->y - 28.0f), 0xff9da7b3, 16.0f, title, (u32)strlen(title));
    }
    cont();
}
