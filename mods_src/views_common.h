#ifndef VIEWS_COMMON_H
#define VIEWS_COMMON_H
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 id_len, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
#include "../dxgfx.h"

#define VIEW_MAX 32
#define NAME_SIZE 17.0f
#define TITLE_SIZE 16.0f
#define SUM_SIZE 15.0f
#define NAME_GAP 12.0f
#define PAD_X 14.0f
#define MIN_HIT_W 48.0f

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
static Entry g_entries[2048];
static u32 g_entry_count;
static int g_index_loaded;
static u32 g_name_map[4096];

static u32 tok_hash(const u8 *tok) {
    u32 h = 2166136261u;
    for (int i = 0; i < 32; i++) { h ^= tok[i]; h *= 16777619u; }
    return h;
}
static void rebuild_name_map(void) {
    for (u32 i = 0; i < 4096; i++) g_name_map[i] = 0xffffffffu;
    for (u32 i = 0; i < g_entry_count; i++) {
        u32 h = tok_hash(g_entries[i].token) & 4095u;
        for (u32 n = 0; n < 4096; n++) {
            u32 s = (h + n) & 4095u;
            if (g_name_map[s] == 0xffffffffu) { g_name_map[s] = i; break; }
        }
    }
}
static void load_index(void) {
    if (g_index_loaded) return;
    g_index_loaded = 1;
    const char *paths[] = { "instruction_names.bin", "./instruction_names.bin", 0 };
    for (int p = 0; paths[p]; p++) {
        FILE *f = fopen(paths[p], "rb");
        if (!f) continue;
        fread(&g_entry_count, 4, 1, f);
        if (g_entry_count > 2048) g_entry_count = 2048;
        g_entry_count = (u32)fread(g_entries, sizeof(Entry), g_entry_count, f);
        fclose(f);
        rebuild_name_map();
        return;
    }
}
static int same_key(const u8 *a, const u8 *b) {
    for (int i = 0; i < 32; i++) if (a[i] != b[i]) return 0;
    return 1;
}
static int zero_key(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}
static void zero_view(View *v) {
    memset(v, 0, sizeof(*v));
    v->parent = -1;
}
static float measure_str(float size, const char *s) {
    if (!s || !s[0]) return 0.0f;
    float out[2] = {0.0f, 0.0f};
    dxgfx_measure_text(size, s, (u32)strlen(s), out);
    return out[0];
}
static const char *token_name(const u8 *tok) {
    static char hex[12];
    if (zero_key(tok)) return "<end>";
    load_index();
    u32 h = tok_hash(tok) & 4095u;
    for (u32 n = 0; n < 4096; n++) {
        u32 s = (h + n) & 4095u;
        u32 idx = g_name_map[s];
        if (idx == 0xffffffffu) break;
        if (!memcmp(g_entries[idx].token, tok, 32)) return g_entries[idx].name;
    }
    snprintf(hex, sizeof(hex), "%02x%02x%02x%02x", tok[0], tok[1], tok[2], tok[3]);
    return hex;
}
/* payload is open-target hash for these instruction names */
static int is_hash_carrier(const u8 *tok) {
    const char *nm = token_name(tok);
    if (!nm || !nm[0] || nm[0] == '<') return 0;
    if (!strcmp(nm, "cond_payload")) return 1;
    if (!strcmp(nm, "jump_payload")) return 1;
    if (!strcmp(nm, "exec_payload")) return 1;
    if (!strcmp(nm, "cond_reexec")) return 1;
    return 0;
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
static float row_text_width(const u8 *instr) {
    const char *nm = token_name(instr);
    float icon = NAME_SIZE;
    float w = 4.0f + 4.0f;
    int is_var = 0;
    if (nm) {
        if (!strcmp(nm, "var_set_payload") || !strcmp(nm, "var_read_payload") ||
            !strcmp(nm, "var_write_payload") || !strcmp(nm, "var_set") ||
            !strcmp(nm, "var_read") || !strcmp(nm, "var_write"))
            is_var = 1;
    }
    if (is_var) {
        w += icon + 6.0f;
        char sum[100];
        payload_summary(instr, sum, sizeof(sum));
        if (sum[0]) w += measure_str(SUM_SIZE, sum);
        else w += 80.0f;
    } else {
        w += measure_str(NAME_SIZE, nm) + 6.0f + icon;
        char sum[100];
        payload_summary(instr, sum, sizeof(sum));
        if (sum[0]) w += NAME_GAP + measure_str(SUM_SIZE, sum);
    }
    w += PAD_X;
    if (w < MIN_HIT_W) w = MIN_HIT_W;
    return w;
}
static float title_text_width(u32 vi, const View *v) {
    char title[80];
    snprintf(title, sizeof(title), "[%u] %02x%02x%02x%02x",
             vi, v->key[0], v->key[1], v->key[2], v->key[3]);
    float w = measure_str(TITLE_SIZE, title) + 16.0f;
    if (w < 72.0f) w = 72.0f;
    return w;
}
static const u8 *row_instr(const View *v, u32 row, u32 *out_count) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0, r = 0;
    while (o + 36 <= nlen && !zero_key(b + o)) {
        u32 pn = *(u32 *)(b + o + 32);
        if (o + 36 + pn > nlen) break;
        if (r == row) {
            if (out_count) *out_count = r + 1;
            return b + o;
        }
        o += 36 + pn;
        r++;
        if (r > 256) break;
    }
    if (out_count) *out_count = r;
    return 0;
}
static u32 block_row_count(const View *v) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0, r = 0;
    while (o + 36 <= nlen && !zero_key(b + o)) {
        u32 pn = *(u32 *)(b + o + 32);
        if (o + 36 + pn > nlen) break;
        o += 36 + pn;
        r++;
        if (r > 256) break;
    }
    return r;
}
static float row_hit_width(const View *v, int row) {
    if (row < 0) return MIN_HIT_W;
    const u8 *instr = row_instr(v, (u32)row, 0);
    if (!instr) {
        float w = measure_str(16.0f, "<end>") + PAD_X;
        if (w < MIN_HIT_W) w = MIN_HIT_W;
        return w;
    }
    return row_text_width(instr);
}
/* views var id is arbitrary-size binary (prefer plaintext strings).
 * Payload layout for views_* mods:
 *   id_len[u32] + id[id_len] + op_args...
 * Legacy: if first u32 is not a plausible id_len, treat first 32 bytes as id.
 */
static u8 *blob(const u8 *id, u32 id_len, u32 *size_out) {
    u32 size = 0;
    u8 *p = cvm_var_get(id, id_len, &size);
    if (!p || size < (u32)sizeof(Table)) return 0;
    if (size_out) *size_out = size;
    return p;
}
static Table *load_table(const u8 *id, u32 id_len) {
    return (Table *)blob(id, id_len, 0);
}
static void store_table(const u8 *id, u32 id_len, Table *t) {
    cvm_var_write(id, id_len, (const u8 *)t, (u32)sizeof(Table));
}
/* Parse payload id. Writes pointer into payload buffer (valid for this call). */
static int payload_id(const u8 **id_out, u32 *id_len_out, const u8 **args, u32 *an) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 4) return 0;
    u32 id_len = *(u32 *)p;
    if (id_len > 0 && id_len <= 256 && n >= 4 + id_len) {
        *id_out = p + 4;
        *id_len_out = id_len;
        if (args) *args = p + 4 + id_len;
        if (an) *an = n - 4 - id_len;
        return 1;
    }
    /* legacy fixed-32 id */
    if (n < 32) return 0;
    *id_out = p;
    *id_len_out = 32;
    if (args) *args = p + 32;
    if (an) *an = n - 32;
    return 1;
}
static Table *load_or_empty(const u8 *id, u32 id_len, int create) {
    Table *tp = load_table(id, id_len);
    if (tp) return tp;
    if (!create) return 0;
    cvm_var_set(id, id_len, (u32)sizeof(Table));
    Table empty;
    memset(&empty, 0, sizeof(empty));
    empty.dragging = -1;
    cvm_var_write(id, id_len, (const u8 *)&empty, (u32)sizeof(Table));
    return load_table(id, id_len);
}

/* Resolve open target for instruction at offset o in cached block.
 * If token is hash-carrier and payload is 32-byte non-zero, use payload. */
static void instr_open_key(const u8 *b, u32 nlen, u32 o, u8 key_out[32]) {
    memset(key_out, 0, 32);
    if (o + 32 > nlen || zero_key(b + o)) return;
    memcpy(key_out, b + o, 32);
    if (o + 36 > nlen) return;
    u32 pn = *(u32 *)(b + o + 32);
    if (pn == 32 && o + 68 <= nlen && is_hash_carrier(key_out)) {
        u8 ph[32];
        memcpy(ph, b + o + 36, 32);
        if (!zero_key(ph)) memcpy(key_out, ph, 32);
    }
}

/* Walk instruction stream of view key to row offset; returns o or nlen on fail. */
static u32 block_row_offset(const View *v, u32 row) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0;
    for (u32 r = 0; r < row && o + 36 <= nlen; r++) {
        if (zero_key(b + o)) return nlen;
        u32 pn = *(u32 *)(b + o + 32);
        if (o + 36 + pn > nlen) return nlen;
        o += 36 + pn;
    }
    return o;
}
#endif
