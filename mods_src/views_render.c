#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) int cvm_has_dll(H h);
extern __declspec(dllimport) int cvm_cache_hit(const H k);

#include "../dxgfx.h"

#define VIEW_MAX 32
typedef struct {
    u8 key[32];
    float x, y;
    int parent;
    int linked;
    float link_x, link_y;
    u32 used;
    u32 cursor;
    u32 pad0, pad1;
} View;
typedef struct {
    u32 count;
    u32 active;
    int dragging;
    u32 pad;
    View views[VIEW_MAX];
} Table;

typedef struct { H token; char name[96]; } Entry;
static Entry entries[2048];
static u32 entry_count;
static int loaded;
static u32 name_map[4096];

/* known specialized native tokens (filled after name index load by name) */
static H tok_var_set_payload, tok_var_read_payload, tok_var_write_payload;
static H tok_var_set, tok_var_read, tok_var_write;
static H tok_const_payload, tok_f32_const;
static H tok_key_pressed, tok_key_down;
static H tok_cond_payload;
static int toks_ready;

/* Colors:
 *  DLL hit           -> cyan text
 *  user override     -> amber text
 *  both              -> magenta text
 *  neither (logical) -> default light
 *  left swatch bar also encodes the same flags.
 */
#define COL_DEFAULT   0xffe8ecef
#define COL_DLL       0xff5ec8e8   /* cyan-ish: native DLL present */
#define COL_OVERRIDE  0xffe0a050   /* amber: user override present */
#define COL_BOTH      0xffd080e0   /* magenta: DLL + override (surface) */
#define COL_SUM       0xff7fb8d8
#define COL_VAR_ID    0xffc8e0a0
#define COL_VAR_SIZE  0xffe8c878
#define COL_END       0xff66717d
#define COL_SW_DLL    0xff3aa0c8
#define COL_SW_OVR    0xffc88830
#define COL_SW_BOTH   0xffb060c0
#define COL_SW_NONE   0xff3a424a

static u32 tok_hash(const u8 *tok) {
    u32 h = 2166136261u;
    for (int i = 0; i < 32; i++) { h ^= tok[i]; h *= 16777619u; }
    return h;
}

static void rebuild_name_map(void) {
    for (u32 i = 0; i < 4096; i++) name_map[i] = 0xffffffffu;
    for (u32 i = 0; i < entry_count; i++) {
        u32 h = tok_hash(entries[i].token) & 4095u;
        for (u32 n = 0; n < 4096; n++) {
            u32 s = (h + n) & 4095u;
            if (name_map[s] == 0xffffffffu) { name_map[s] = i; break; }
        }
    }
}

static int find_token_by_name(const char *name, H out) {
    for (u32 i = 0; i < entry_count; i++) {
        if (!strcmp(entries[i].name, name)) {
            memcpy(out, entries[i].token, 32);
            return 1;
        }
    }
    return 0;
}

static void load_index(void) {
    if (loaded) return;
    loaded = 1;
    const char *paths[] = { "instruction_names.bin", "./instruction_names.bin", 0 };
    for (int p = 0; paths[p]; p++) {
        FILE *f = fopen(paths[p], "rb");
        if (!f) continue;
        fread(&entry_count, 4, 1, f);
        if (entry_count > 2048) entry_count = 2048;
        entry_count = (u32)fread(entries, sizeof(Entry), entry_count, f);
        fclose(f);
        rebuild_name_map();
        /* resolve specialized tokens by name */
        find_token_by_name("var_set_payload", tok_var_set_payload);
        find_token_by_name("var_read_payload", tok_var_read_payload);
        find_token_by_name("var_write_payload", tok_var_write_payload);
        find_token_by_name("var_set", tok_var_set);
        find_token_by_name("var_read", tok_var_read);
        find_token_by_name("var_write", tok_var_write);
        find_token_by_name("const_payload", tok_const_payload);
        find_token_by_name("f32_const", tok_f32_const);
        find_token_by_name("key_pressed", tok_key_pressed);
        find_token_by_name("key_down", tok_key_down);
        find_token_by_name("cond_payload", tok_cond_payload);
        toks_ready = 1;
        return;
    }
}

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

static int same32(const u8 *a, const u8 *b) {
    return !memcmp(a, b, 32);
}

static const char *token_name(const u8 *tok) {
    static char hex[12];
    if (zero32(tok)) return "<end>";
    load_index();
    u32 h = tok_hash(tok) & 4095u;
    for (u32 n = 0; n < 4096; n++) {
        u32 s = (h + n) & 4095u;
        u32 idx = name_map[s];
        if (idx == 0xffffffffu) break;
        if (!memcmp(entries[idx].token, tok, 32)) return entries[idx].name;
    }
    snprintf(hex, sizeof(hex), "%02x%02x%02x%02x", tok[0], tok[1], tok[2], tok[3]);
    return hex;
}

static float measure(float size, const char *s) {
    if (!s || !s[0]) return 0.0f;
    float out[2] = {0.0f, 0.0f};
    dxgfx_measure_text(size, s, (u32)strlen(s), out);
    return out[0];
}

/* Classify token WITHOUT network I/O (render hot path).
 * bit0 = native DLL present
 * bit1 = resolved/override content present in local block cache
 * (server override is installed into cache on open/resolve; do not uget here —
 *  concurrent use of the main conn corrupts the instruction stream.)
 */
static int token_flags(const u8 *tok) { (void)tok; return 0; }

static u32 color_for_flags(int flags) {
    if (flags == 3) return COL_BOTH;
    if (flags == 1) return COL_DLL;
    if (flags == 2) return COL_OVERRIDE;
    return COL_DEFAULT;
}

static u32 swatch_for_flags(int flags) {
    if (flags == 3) return COL_SW_BOTH;
    if (flags == 1) return COL_SW_DLL;
    if (flags == 2) return COL_SW_OVR;
    return COL_SW_NONE;
}

/* Format binary id for display: printable string if all printable, else hex (truncated). */
static void fmt_id(const u8 *id, u32 n, char *out, u32 outn) {
    if (!n) { snprintf(out, outn, "<>"); return; }
    int printable = 1;
    for (u32 i = 0; i < n; i++) if (id[i] < 32 || id[i] > 126) { printable = 0; break; }
    if (printable) {
        u32 z = n < outn - 3 ? n : outn - 3;
        snprintf(out, outn, "%.*s", (int)z, (const char *)id);
        return;
    }
    /* hex, max 12 bytes shown */
    u32 show = n < 12 ? n : 12;
    u32 pos = 0;
    for (u32 i = 0; i < show && pos + 2 < outn; i++) {
        pos += (u32)snprintf(out + pos, outn - pos, "%02x", id[i]);
    }
    if (n > show && pos + 2 < outn) snprintf(out + pos, outn - pos, "..");
}

/* Parse specialized var payload layouts into display strings.
 * Returns 1 if specialized handled (out_id/out_extra filled).
 * var_set_payload: id_len + id + size[u32]  OR id_len + id + data  OR legacy id32+...
 * var_read_payload: entire payload = id
 * var_write_payload: id_len + id [+ data] OR legacy
 */
static int specialized_var(const u8 *tok, const u8 *payload, u32 pn,
                           char *id_text, u32 id_n,
                           char *extra_text, u32 extra_n,
                           const char **icon_name) {
    load_index();
    if (!toks_ready) return 0;
    id_text[0] = 0;
    extra_text[0] = 0;
    *icon_name = 0;

    if (same32(tok, tok_var_read_payload)) {
        *icon_name = "var_read_payload";
        fmt_id(payload, pn, id_text, id_n);
        return 1;
    }
    if (same32(tok, tok_var_write_payload)) {
        *icon_name = "var_write_payload";
        if (pn >= 4) {
            u32 id_len = *(u32*)payload;
            if (id_len > 0 && id_len <= 256 && pn >= 4 + id_len) {
                fmt_id(payload + 4, id_len, id_text, id_n);
                u32 rest = pn - 4 - id_len;
                if (rest) snprintf(extra_text, extra_n, "%uB", rest);
                return 1;
            }
        }
        /* legacy */
        if (pn >= 32) {
            fmt_id(payload, 32, id_text, id_n);
            if (pn > 32) snprintf(extra_text, extra_n, "%uB", pn - 32);
            return 1;
        }
        fmt_id(payload, pn, id_text, id_n);
        return 1;
    }
    if (same32(tok, tok_var_set_payload)) {
        *icon_name = "var_set_payload";
        if (pn >= 4) {
            u32 id_len = *(u32*)payload;
            if (id_len > 0 && id_len <= 256 && pn >= 4 + id_len) {
                fmt_id(payload + 4, id_len, id_text, id_n);
                u32 rest = pn - 4 - id_len;
                if (rest == 4) {
                    u32 sz = *(u32*)(payload + 4 + id_len);
                    snprintf(extra_text, extra_n, "%u", sz);
                } else if (rest) {
                    snprintf(extra_text, extra_n, "%uB", rest);
                } else {
                    snprintf(extra_text, extra_n, "0");
                }
                return 1;
            }
        }
        /* legacy id[32]+size[u32] or id[32]+data */
        if (pn == 36) {
            fmt_id(payload, 32, id_text, id_n);
            snprintf(extra_text, extra_n, "%u", *(u32*)(payload + 32));
            return 1;
        }
        if (pn >= 32) {
            fmt_id(payload, 32, id_text, id_n);
            if (pn > 32) snprintf(extra_text, extra_n, "%uB", pn - 32);
            return 1;
        }
        return 0;
    }
    /* stack var ops have no payload id; just icon */
    if (same32(tok, tok_var_set)) { *icon_name = "var_set"; snprintf(id_text, id_n, "set"); return 1; }
    if (same32(tok, tok_var_read)) { *icon_name = "var_read"; snprintf(id_text, id_n, "read"); return 1; }
    if (same32(tok, tok_var_write)) { *icon_name = "var_write"; snprintf(id_text, id_n, "write"); return 1; }
    return 0;
}

/* Other high-value specialized summaries */
static int specialized_other(const u8 *tok, const u8 *payload, u32 pn,
                             char *sum, u32 sumn, const char **icon_name) {
    load_index();
    if (!toks_ready) return 0;
    sum[0] = 0;
    if (same32(tok, tok_const_payload)) {
        *icon_name = "const_payload";
        if (pn == 4) {
            u32 v = *(u32*)payload;
            float f = *(float*)payload;
            /* prefer int if looks integral */
            if (v < 0x10000u || (v & 0xff000000u) == 0)
                snprintf(sum, sumn, "%u", v);
            else
                snprintf(sum, sumn, "%g", (double)f);
            return 1;
        }
        if (pn == 0) { snprintf(sum, sumn, "empty"); return 1; }
        snprintf(sum, sumn, "[%uB]", pn);
        return 1;
    }
    if (same32(tok, tok_f32_const)) {
        *icon_name = "f32_const";
        if (pn >= 4) snprintf(sum, sumn, "%g", (double)*(float*)payload);
        return 1;
    }
    if (same32(tok, tok_key_pressed) || same32(tok, tok_key_down)) {
        *icon_name = same32(tok, tok_key_pressed) ? "key_pressed" : "key_down";
        if (pn >= 4) {
            u32 vk = *(u32*)payload;
            if (vk >= 32 && vk < 127) snprintf(sum, sumn, "'%c'", (char)vk);
            else snprintf(sum, sumn, "VK_%02X", vk);
        }
        return 1;
    }
    if (same32(tok, tok_cond_payload)) {
        *icon_name = "cond_payload";
        if (pn >= 32) {
            /* show target token short name */
            const char *nm = token_name(payload);
            snprintf(sum, sumn, "-> %s", nm);
        }
        return 1;
    }
    return 0;
}

static void payload_summary_generic(const u8 *instr, char *out, u32 outn) {
    u32 pn = *(u32 *)(instr + 32);
    if (!pn) { out[0] = 0; return; }
    const u8 *p = instr + 36;
    u32 z = pn < 42 ? pn : 42;
    int printable = 1;
    for (u32 j = 0; j < z; j++) if (p[j] < 32 || p[j] > 126) { printable = 0; break; }
    if (printable) snprintf(out, outn, "'%.*s'", (int)z, (const char *)p);
    else snprintf(out, outn, "[%u bytes]", pn);
}

/* Layout constants — keep hit-testing in views.c in sync conceptually. */
#define NAME_SIZE 17.0f
#define SUM_SIZE 15.0f
#define NAME_GAP 10.0f
#define ICON_GAP 6.0f
#define PAD_X 14.0f
#define MIN_HIT_W 48.0f
#define SWATCH_W 4.0f

/* payload: views_var_id[32]
 * Renders every used view with:
 *  - left color swatch (DLL / override / both)
 *  - token name colored by same flags
 *  - token.svg (or geometric fallback) to the RIGHT of the name
 *  - specialized summaries for var_* / const / key / cond
 */
__declspec(dllexport) void run(void) {
    if (cvm_payload_size() < 32) { cont(); return; }
    H id; memcpy(id, cvm_payload(), 32);
    u32 size = 0;
    u8 *raw = cvm_var_get(id, 32, &size);
    if (!raw || size < sizeof(Table)) { cont(); return; }
    Table *t = (Table *)raw;

    for (u32 vi = 0; vi < t->count; vi++) {
        View *v = &t->views[vi];
        if (!v->used) continue;

        if (v->linked && v->parent >= 0 && (u32)v->parent < t->count && t->views[v->parent].used) {
            View *p = &t->views[v->parent];
            dxgfx_draw_line(p->x + v->link_x, p->y + v->link_y, v->x, v->y, 0xff62c982, 2.0f);
        }

        char title[80];
        snprintf(title, sizeof(title), "[%u] %02x%02x%02x%02x",
                 vi, v->key[0], v->key[1], v->key[2], v->key[3]);
        float title_w = measure(16.0f, title);
        if (vi == t->active) {
            float tw = title_w + 16.0f;
            if (tw < 72.0f) tw = 72.0f;
            dxgfx_draw_rect(v->x - 6.0f, v->y - 30.0f, tw, 22.0f, 0xff2a333c, 1.0f, 1);
        }
        dxgfx_draw_text((int)v->x, (int)(v->y - 28.0f), 0xff9da7b3, 16.0f, title, (u32)strlen(title));

        H h;
        cvm_resolve_payload_hash(v->key, h);
        u8 *b = cvm_cached_base();
        u32 n = cvm_cached_len();
        u32 o = 0;
        u32 row = 0;
        float ry = v->y;
        while (o + 36 <= n && !zero32(b + o)) {
            u32 pn = *(u32 *)(b + o + 32);
            if (o + 36 + pn > n) break;
            const u8 *tok = b + o;
            const u8 *payload = b + o + 36;
            const char *nm = token_name(tok);
            int flags = token_flags(tok);
            u32 name_col = color_for_flags(flags);
            u32 sw_col = swatch_for_flags(flags);

            float icon_sz = dxgfx_icon_size(NAME_SIZE);
            float name_w = measure(NAME_SIZE, nm);

            char id_text[96]; char extra_text[64]; char sum[100];
            const char *icon_name = nm; /* default: name.svg */
            int is_var = 0;
            id_text[0] = extra_text[0] = sum[0] = 0;

            if (specialized_var(tok, payload, pn, id_text, sizeof(id_text),
                                extra_text, sizeof(extra_text), &icon_name)) {
                is_var = 1;
                if (!icon_name) icon_name = nm;
            } else if (specialized_other(tok, payload, pn, sum, sizeof(sum), &icon_name)) {
                if (!icon_name) icon_name = nm;
            } else {
                payload_summary_generic(b + o, sum, sizeof(sum));
                icon_name = nm;
            }

            float sum_w = 0.0f;
            if (is_var) {
                if (id_text[0]) sum_w += measure(SUM_SIZE, id_text);
                if (extra_text[0]) sum_w += NAME_GAP + measure(SUM_SIZE, extra_text);
            } else if (sum[0]) {
                sum_w = measure(SUM_SIZE, sum);
            }

            float gap = (sum_w > 0.0f || is_var) ? NAME_GAP : 0.0f;
            float total_w = SWATCH_W + 4.0f + name_w + ICON_GAP + icon_sz + gap + sum_w + PAD_X;
            if (total_w < MIN_HIT_W) total_w = MIN_HIT_W;

            int selected = (vi == t->active && row == v->cursor);
            if (selected) dxgfx_draw_rect(v->x - 7.0f, ry - 2.0f, total_w, 23.0f, 0xff34414d, 1.0f, 1);

            /* left status swatch */
            dxgfx_draw_rect(v->x - 6.0f, ry + 2.0f, SWATCH_W, 14.0f, sw_col, 1.0f, 1);

            float tx = v->x + SWATCH_W + 2.0f;
            /* specialized var: ICON first (no text token name), then id + size */
            if (is_var) {
                dxgfx_draw_icon(tx, ry + 1.0f, icon_sz, name_col, icon_name);
                float cx = tx + icon_sz + ICON_GAP;
                if (id_text[0]) {
                    dxgfx_draw_text((int)cx, (int)ry, COL_VAR_ID, SUM_SIZE, id_text, (u32)strlen(id_text));
                    cx += measure(SUM_SIZE, id_text) + NAME_GAP;
                }
                if (extra_text[0]) {
                    dxgfx_draw_text((int)cx, (int)ry, COL_VAR_SIZE, SUM_SIZE, extra_text, (u32)strlen(extra_text));
                }
            } else {
                dxgfx_draw_text((int)tx, (int)ry, name_col, NAME_SIZE, nm, (u32)strlen(nm));
                float ix = tx + name_w + ICON_GAP;
                dxgfx_draw_icon(ix, ry + 1.0f, icon_sz, name_col, icon_name);
                if (sum[0]) {
                    int px = (int)(ix + icon_sz + NAME_GAP);
                    dxgfx_draw_text(px, (int)ry, COL_SUM, SUM_SIZE, sum, (u32)strlen(sum));
                }
            }

            ry += 24.0f;
            o += 36 + pn;
            row++;
            if (row > 256) break;
        }
        {
            float end_w = measure(16.0f, "<end>") + 14.0f;
            if (end_w < 48.0f) end_w = 48.0f;
            int end_sel = (vi == t->active && row == v->cursor);
            if (end_sel) dxgfx_draw_rect(v->x - 7.0f, ry - 2.0f, end_w, 23.0f, 0xff34414d, 1.0f, 1);
            dxgfx_draw_text((int)v->x, (int)ry, COL_END, 16.0f, "<end>", 5);
        }
    }
    cont();
}
