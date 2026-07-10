#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 *size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);

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
/* open-address map: index into entries, 0xffffffff empty */
static u32 name_map[4096];

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
        return;
    }
}

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
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

static void payload_summary(const u8 *instr, char *out, u32 outn) {
    u32 pn = *(u32 *)(instr + 32);
    if (!pn) { out[0] = 0; return; }
    const u8 *p = instr + 36;
    u32 z = pn < 42 ? pn : 42;
    int printable = 1;
    for (u32 j = 0; j < z; j++) if (p[j] < 32 || p[j] > 126) { printable = 0; break; }
    if (printable) snprintf(out, outn, "'%.*s'", (int)z, (const char *)p);
    else snprintf(out, outn, "[%u bytes]", pn);
}

/* payload: views_var_id[32]
 * Renders every used view: parent link, title, rows, selection, <end>.
 * Row layout is text-width based: payload summary trails the token name.
 */
__declspec(dllexport) void run(void) {
    if (cvm_payload_size() < 32) { cont(); return; }
    H id; memcpy(id, cvm_payload(), 32);
    u32 size = 0;
    u8 *raw = cvm_var_get(id, &size);
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
            const char *nm = token_name(b + o);
            float name_w = measure(17.0f, nm);
            char sum[100];
            payload_summary(b + o, sum, sizeof(sum));
            float sum_w = sum[0] ? measure(15.0f, sum) : 0.0f;
            float gap = sum[0] ? 12.0f : 0.0f;
            float total_w = name_w + gap + sum_w + 14.0f;
            if (total_w < 48.0f) total_w = 48.0f;

            int selected = (vi == t->active && row == v->cursor);
            if (selected) dxgfx_draw_rect(v->x - 7.0f, ry - 2.0f, total_w, 23.0f, 0xff34414d, 1.0f, 1);

            dxgfx_draw_text((int)v->x, (int)ry, 0xffe8ecef, 17.0f, nm, (u32)strlen(nm));
            if (sum[0]) {
                int px = (int)(v->x + name_w + gap);
                dxgfx_draw_text(px, (int)ry, 0xff7fb8d8, 15.0f, sum, (u32)strlen(sum));
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
            dxgfx_draw_text((int)v->x, (int)ry, 0xff66717d, 16.0f, "<end>", 5);
        }
    }
    cont();
}
