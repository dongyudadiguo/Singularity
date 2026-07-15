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
extern __declspec(dllimport) u32 cvm_children(const H parent, H *out, u32 cap);
extern __declspec(dllimport) u32 cvm_file_read(const H h, u8 *out, u32 cap);
#include "../dxgfx.h"
#include "block_layout.h"

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

#define INDENT_X 22.0f
/* Depth from parent chain (0 = root). Caps at 16. */
static int view_depth(const Table *t, u32 vi) {
    int d = 0;
    int guard = 0;
    int p = (int)vi;
    while (p >= 0 && (u32)p < t->count && t->views[p].used && guard++ < 16) {
        if (t->views[p].parent < 0) break;
        d++;
        p = t->views[p].parent;
    }
    return d;
}
static float view_indent_x(const Table *t, u32 vi) {
    return (float)view_depth(t, vi) * INDENT_X;
}
/* Draw position for rows/titles = stored x + indent */
static float view_draw_x(const Table *t, u32 vi) {
    return t->views[vi].x + view_indent_x(t, vi);
}

/* forward decls for tag helpers defined below */
static void key_display_name(const u8 *key, char *out, u32 outn);
static int key_is_tag(const u8 *key);
static u32 tag_child_count(const u8 *parent);
static u32 tag_child_at(const u8 *parent, u32 row, u8 child_out[32]);
static float tag_row_hit_width(const u8 *child_key);
static void view_row_open_key(const View *v, u32 row, u8 key_out[32]);
static float title_text_width(u32 vi, const View *v);
static float row_text_width(const u8 *instr);

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
    if (!strcmp(nm, "cond_token_payload")) return 1;
    if (!strcmp(nm, "token_run_by_hand")) return 1;
    if (!strcmp(nm, "cond_payload")) return 1;
    if (!strcmp(nm, "jump_payload")) return 1;
    if (!strcmp(nm, "exec_payload")) return 1;
    /* cond_reexec has no target key in payload — opens self / not carrier */
    return 0;
}
/* token display name from instruction pointer (new layout) */
static const char *instr_token_name(const u8 *instr) {
    static char hex[16];
    if (!instr || bl_is_end(instr)) return "<end>";
    u32 tlen = bl_tlen(instr);
    const u8 *tok = bl_token_c(instr);
    if (tlen == 32) return token_name(tok);
    /* non-32: show short hex of first bytes */
    snprintf(hex, sizeof(hex), "d:%u", tlen);
    return hex;
}
static const u8 *instr_token_ptr(const u8 *instr, u32 *tlen_out) {
    if (!instr || bl_is_end(instr)) { if (tlen_out) *tlen_out = 0; return 0; }
    if (tlen_out) *tlen_out = bl_tlen(instr);
    return bl_token_c(instr);
}
static const u8 *instr_payload_ptr(const u8 *instr, u32 *plen_out) {
    if (!instr || bl_is_end(instr)) { if (plen_out) *plen_out = 0; return 0; }
    if (plen_out) *plen_out = bl_plen(instr);
    return bl_payload_c(instr);
}


/* Header chrome buttons to the right of title text.
 * layout: [title]  [提交] [合闸] / [推荐]
 * pad0 bit0 = latch.
 */
#define BTN_H 18.0f
#define BTN_GAP 6.0f
#define BTN_PAD 8.0f
static float btn_w(const char *label) {
    float w = measure_str(13.0f, label) + 12.0f;
    if (w < 36.0f) w = 36.0f;
    return w;
}
/* Returns total header width including buttons (for hit tests). */
static float header_total_width(u32 vi, const View *v, int dirty, int show_rec) {
    float tw = title_text_width(vi, v);
    float x = tw + 10.0f;
    int latch = (v->pad0 & 1u) != 0;
    if (dirty && !latch) x += btn_w("commit") + BTN_GAP;
    x += btn_w(latch ? "latch*" : "latch") + BTN_GAP;
    if (show_rec && !dirty) x += btn_w("vote") + BTN_GAP;
    return x;
}
/* Which header button under (mx,my)? 0=none 1=commit 2=latch 3=vote
 * show_latch=0 for tag explorer nodes.
 */
static int header_btn_hit(const Table *t, u32 vi, const View *v, float mx, float my, float title_h,
                          int dirty, int show_rec, int show_latch) {
    if (my < v->y - title_h || my >= v->y) return 0;
    float tw = title_text_width(vi, v);
    float base = t ? view_draw_x(t, vi) : v->x;
    float x = base + tw + 10.0f;
    float by = v->y - title_h + 4.0f;
    int latch = (v->pad0 & 1u) != 0;
    if (show_latch && dirty && !latch) {
        float w = btn_w("commit");
        if (mx >= x && mx < x + w && my >= by && my < by + BTN_H) return 1;
        x += w + BTN_GAP;
    }
    if (show_latch) {
        float w = btn_w(latch ? "latch*" : "latch");
        if (mx >= x && mx < x + w && my >= by && my < by + BTN_H) return 2;
        x += w + BTN_GAP;
    }
    if (show_rec) {
        float w = btn_w("vote");
        if (mx >= x && mx < x + w && my >= by && my < by + BTN_H) return 3;
    }
    return 0;
}

/* Parse cond_token_payload layout. Returns 1 if token-like. */
static int cond_token_parse(const u8 *payload, u32 pn, u8 tok_out[32], u32 *uid, u8 *once, u8 *conti) {
    if (pn < 32) return 0;
    if (tok_out) memcpy(tok_out, payload, 32);
    if (uid) *uid = (pn >= 36) ? *(u32*)(payload + 32) : 0;
    if (once) *once = (pn >= 38) ? payload[36] : 0;
    if (conti) *conti = (pn >= 38) ? payload[37] : 0;
    return 1;
}

static void payload_summary(const u8 *instr, char *out, u32 outn) {
    u32 pn = 0;
    const u8 *p = instr_payload_ptr(instr, &pn);
    if (!pn || !p) { out[0] = 0; return; }
    u32 z = pn < 42 ? pn : 42;
    int printable = 1;
    for (u32 j = 0; j < z; j++) if (p[j] < 32 || p[j] > 126) { printable = 0; break; }
    if (printable) snprintf(out, outn, "'%.*s'", (int)z, (const char *)p);
    else snprintf(out, outn, "[%u bytes]", pn);
}
static float row_text_width(const u8 *instr) {
    const char *nm = instr_token_name(instr);
    float icon = NAME_SIZE;
    float w = 2.0f; /* no left swatch */
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
    char dname[80];
    key_display_name(v->key, dname, sizeof(dname));
    char title[120];
    snprintf(title, sizeof(title), "[%u] %s", vi, dname);
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
    while (bl_ok(b, nlen, o) && !bl_is_end(b + o)) {
        if (r == row) {
            if (out_count) *out_count = r + 1;
            return b + o;
        }
        o += bl_instr_size(b + o);
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
    while (bl_ok(b, nlen, o) && !bl_is_end(b + o)) {
        o += bl_instr_size(b + o);
        r++;
        if (r > 256) break;
    }
    return r;
}
static float row_hit_width(const View *v, int row) {
    if (row < 0) return MIN_HIT_W;
    if (key_is_tag(v->key)) {
        u8 child[32];
        if (!tag_child_at(v->key, (u32)row, child)) {
            float w = measure_str(16.0f, "<end>") + PAD_X;
            if (w < MIN_HIT_W) w = MIN_HIT_W;
            return w;
        }
        return tag_row_hit_width(child);
    }
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
    if (!bl_ok(b, nlen, o) || bl_is_end(b + o)) return;
    u32 tlen = bl_tlen(b + o);
    const u8 *tok = bl_token_c(b + o);
    u32 plen = bl_plen(b + o);
    const u8 *pay = bl_payload_c(b + o);
    /* Default open target = token if 32-byte key */
    if (tlen == 32) memcpy(key_out, tok, 32);
    /* Hash-carriers open TARGET in payload */
    if (tlen == 32 && is_hash_carrier(tok) && pay && plen >= 32) {
        const char *nm = token_name(tok);
        if (nm && !strcmp(nm, "token_run_by_hand") && plen >= 36) {
            if (!zero_key(pay + 4)) memcpy(key_out, pay + 4, 32);
        } else if (!zero_key(pay)) {
            memcpy(key_out, pay, 32);
        }
    }
}

/* Walk instruction stream of view key to row offset; returns o or nlen on fail. */
static u32 block_row_offset(const View *v, u32 row) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0;
    for (u32 r = 0; r < row && bl_ok(b, nlen, o) && !bl_is_end(b + o); r++) {
        o += bl_instr_size(b + o);
    }
    return o;
}

/* ---- tag-graph view helpers (network token explorer) ----
 * CRITICAL: paint/hit-test is every frame. NEVER call cvm_file_read here —
 * it downloads entire blobs (40KB+ DLLs) on the main conn and freezes the UI
 * (white screen). Only touch disk cache + cvm_children (_ch.bin).
 */
static void tag_root_key(u8 out[32]) {
    static const u8 root[32] = {
        0xac,0x79,0x84,0x37,0x38,0x60,0xa5,0x14,
        0x10,0x56,0x1d,0x1c,0xbc,0x5e,0x1b,0x64,
        0xa3,0x07,0x27,0x75,0xce,0xb8,0x75,0x66,
        0xfc,0x23,0x1c,0xf4,0x18,0x0f,0xb9,0x89
    };
    memcpy(out, root, 32);
}

static int blob_is_tag(const u8 *p, u32 n) {
    if (!n || n >= 96) return 0;
    if (p[0] != '#') return 0;
    for (u32 i = 0; i < n; i++) if (p[i] < 32 || p[i] > 126) return 0;
    return 1;
}

static int blob_is_name(const u8 *p, u32 n) {
    if (!n || n >= 96) return 0;
    if (p[0] == '#') return 0;
    for (u32 i = 0; i < n; i++) if (p[i] < 32 || p[i] > 126) return 0;
    return 1;
}

static void cache_blob_path(const u8 *h, char *path, u32 pathn) {
    snprintf(path, pathn,
        "cache/%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x.bin",
        h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7],
        h[8],h[9],h[10],h[11],h[12],h[13],h[14],h[15],
        h[16],h[17],h[18],h[19],h[20],h[21],h[22],h[23],
        h[24],h[25],h[26],h[27],h[28],h[29],h[30],h[31]);
}

/* Disk-only peek. Returns 0 if not cached locally (NO network). */
static u32 disk_peek(const u8 *key, u8 *out, u32 cap, u32 *full_sz_out) {
    char path[140];
    cache_blob_path(key, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) { if (full_sz_out) *full_sz_out = 0; return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 0; }
    if (full_sz_out) *full_sz_out = (u32)sz;
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    u32 got = 0;
    if (out && cap) got = (u32)fread(out, 1, cap, f);
    fclose(f);
    return got;
}

/* Tiny process-local memo so paint doesn't re-stat the same keys 100x/frame. */
typedef struct {
    u8 key[32];
    char name[80];
    u8 is_tag;   /* 0 unknown/no, 1 yes */
    u8 on;
} TagMemo;
static TagMemo g_tag_memo[128];

static TagMemo *tag_memo_get(const u8 *key) {
    u32 h = (u32)key[0] | ((u32)key[1] << 8) | ((u32)key[2] << 16) | ((u32)key[3] << 24);
    u32 idx = h & 127u;
    for (u32 n = 0; n < 128; n++) {
        u32 i = (idx + n) & 127u;
        if (g_tag_memo[i].on && same_key(g_tag_memo[i].key, key)) return &g_tag_memo[i];
        if (!g_tag_memo[i].on) {
            memset(&g_tag_memo[i], 0, sizeof(g_tag_memo[i]));
            memcpy(g_tag_memo[i].key, key, 32);
            g_tag_memo[i].on = 1;
            return &g_tag_memo[i];
        }
    }
    return 0;
}

static int key_is_tag_root(const u8 *key) {
    u8 root[32]; tag_root_key(root);
    return same_key(key, root);
}

static int known_view_tag_text(const u8 *key, char *out, u32 outn) {
    /* same content-addressed tags as name_common taxonomy */
    typedef struct { u8 key[32]; const char *text; } KT;
    static const KT k[] = {
        {{0xac,0x79,0x84,0x37,0x38,0x60,0xa5,0x14,0x10,0x56,0x1d,0x1c,0xbc,0x5e,0x1b,0x64,0xa3,0x07,0x27,0x75,0xce,0xb8,0x75,0x66,0xfc,0x23,0x1c,0xf4,0x18,0x0f,0xb9,0x89}, "#TAG"},
        {{0xf8,0x0b,0x67,0xf8,0xe0,0x92,0x47,0x67,0x0f,0xa5,0xe3,0xd5,0x25,0x88,0xc3,0x7c,0x29,0x90,0xbd,0x38,0x4c,0x58,0x6b,0x8d,0x4a,0xc1,0xbe,0xae,0xc7,0x32,0xb3,0xfc}, "#atomic"},
        {{0x3b,0x64,0x4d,0xe3,0x77,0xc3,0x2c,0x78,0x79,0x36,0x05,0xa2,0x5a,0xa9,0x15,0xbf,0x23,0x81,0x6b,0xd5,0x75,0xe5,0x53,0x33,0xca,0x97,0x5b,0x82,0x20,0xdb,0xdb,0xe6}, "#ops"},
        {{0x12,0x5a,0x83,0x05,0x4f,0xdc,0x92,0x30,0x25,0x78,0x14,0xd4,0x16,0xd8,0x3d,0x5e,0xde,0x47,0x64,0xd8,0x7d,0x32,0x14,0xfb,0x4c,0xdc,0xba,0xb3,0x79,0x9b,0xbd,0x88}, "#stack"},
        {{0x20,0x3a,0x3e,0xdd,0x75,0xa1,0x4d,0xde,0x05,0x53,0xf0,0x87,0xaa,0x15,0x8e,0x1b,0xf3,0x81,0x0b,0xd1,0x34,0x90,0xb5,0x8e,0xe5,0x0c,0x0e,0xd1,0x2b,0xfb,0x60,0x8f}, "#f32"},
        {{0x78,0x5f,0x48,0x7b,0x44,0xbc,0xb8,0xf3,0xc0,0x71,0x63,0x73,0xa9,0x6a,0x43,0xfd,0xe9,0xe5,0x27,0x20,0xd5,0xd0,0x15,0x79,0x16,0xfa,0xd9,0x64,0xf0,0x77,0x1d,0xc5}, "#i32"},
        {{0xb8,0x54,0x96,0x38,0x7a,0xa7,0xbc,0xa0,0x30,0x26,0x2b,0x42,0x2f,0xf2,0x29,0x96,0x63,0xa6,0x2a,0xfc,0xde,0x46,0x86,0x16,0x7c,0x25,0x9c,0x8f,0x0f,0x06,0xbd,0x11}, "#var"},
        {{0xa0,0xa7,0xb4,0xc2,0x31,0x73,0x53,0xde,0x79,0x2e,0xd9,0xe1,0xef,0x25,0xe1,0xd3,0xd6,0x39,0x0c,0xad,0x68,0x2b,0x0a,0x19,0xf1,0x14,0x08,0xda,0x05,0xfb,0xa0,0x73}, "#block"},
        {{0x55,0x23,0x2a,0x68,0x8c,0x4a,0x81,0x2a,0x63,0xea,0xb0,0x4e,0xf1,0xe7,0x58,0x98,0xe0,0x09,0x75,0xe9,0xaf,0x30,0x87,0x63,0x04,0x31,0x23,0xe5,0xd2,0x46,0xc5,0x6f}, "#control"},
        {{0xe1,0xc9,0x73,0x8e,0xf1,0x72,0x5f,0x86,0xe1,0x44,0x76,0x16,0x3f,0xb3,0xf5,0x52,0x4b,0x7c,0x05,0xed,0x4c,0x8d,0x88,0xa1,0xf4,0xc9,0x34,0x41,0x27,0x0c,0x9f,0xb8}, "#input"},
        {{0x30,0xef,0x15,0xc7,0xb4,0x20,0x85,0x58,0x8f,0x47,0x33,0x58,0x3f,0x16,0xa2,0x7c,0x77,0x6b,0x70,0x8c,0x43,0xb1,0x5a,0x11,0x28,0x98,0xb3,0x7a,0x89,0xfb,0x13,0x8d}, "#gfx"},
        {{0xee,0x9a,0x0d,0xc2,0x80,0x5c,0xea,0x0c,0x50,0x24,0x7d,0xe8,0x8b,0xd7,0x5d,0xc2,0x84,0x5b,0xdf,0xd3,0xae,0x63,0x60,0xa2,0x22,0xf6,0xb7,0xb1,0x04,0x82,0xb1,0x6c}, "#string"},
        {{0x91,0x37,0x16,0x26,0xd9,0xd2,0xa9,0xa1,0x16,0x51,0x34,0xaa,0x9f,0x96,0xe8,0xd5,0x8f,0xcb,0x21,0x7f,0x53,0xe1,0xbb,0x65,0xe2,0xf6,0x13,0xe1,0xaa,0x6a,0x36,0xad}, "#name"},
        {{0x77,0x92,0x1c,0x37,0xdc,0x8a,0xcc,0xce,0x46,0x17,0xa0,0xe6,0x55,0x9c,0x17,0xba,0xbc,0xaf,0x83,0x43,0x5f,0x59,0x53,0x58,0x5b,0x55,0x39,0x66,0xaf,0x69,0x18,0x15}, "#views"},
        {{0x16,0xaf,0x03,0xef,0x0a,0x42,0xe8,0x52,0xcd,0x26,0x39,0xc0,0xa4,0x9c,0x60,0x6c,0x64,0xfa,0xc3,0x83,0x67,0x6e,0x6a,0xe1,0xe5,0x71,0xd2,0x58,0x0b,0x23,0xef,0xae}, "#misc"},
        {{0x3a,0x4c,0xba,0xd1,0x05,0x53,0xdb,0x37,0xe7,0x36,0x1e,0x52,0x24,0xb5,0xe4,0x29,0xad,0xa4,0xb6,0xf7,0xea,0xd0,0x7f,0x1e,0xd4,0x74,0x27,0x5e,0x70,0x27,0x75,0x72}, "#actions"},
        {{0x13,0x4d,0xfa,0x80,0x33,0xc6,0x66,0xd0,0xe0,0x4f,0x41,0xc2,0xcb,0x1a,0xa9,0xb4,0x55,0x65,0x03,0x35,0xb2,0x33,0x95,0xc6,0x29,0x6b,0x5f,0x05,0xcc,0x3c,0x11,0x72}, "#modules"},
    };
    for (u32 i = 0; i < (u32)(sizeof(k)/sizeof(k[0])); i++) {
        if (same_key(k[i].key, key)) {
            if (out && outn) { strncpy(out, k[i].text, outn - 1); out[outn - 1] = 0; }
            return 1;
        }
    }
    return 0;
}

/* True if this key's content is a tag string (#...). Disk-only + root hardcode. */
static int key_is_tag(const u8 *key) {
    if (known_view_tag_text(key, 0, 0)) return 1;
    TagMemo *m = tag_memo_get(key);
    if (m && m->name[0]) return m->is_tag; /* already classified via display */
    u8 buf[96]; u32 full = 0;
    u32 n = disk_peek(key, buf, sizeof(buf), &full);
    if (full >= 96) return 0;          /* large blob => not a tag string */
    if (!n) return 0;                  /* uncached: do NOT network on paint */
    return blob_is_tag(buf, n < full ? n : full);
}

/* Display title: disk cache / children name edge / hex. No network downloads. */
static void key_display_name(const u8 *key, char *out, u32 outn) {
    if (!outn) return;
    out[0] = 0;
    if (known_view_tag_text(key, out, outn)) {
        TagMemo *m = tag_memo_get(key);
        if (m) { strncpy(m->name, out, 79); m->is_tag = 1; }
        return;
    }
    TagMemo *m = tag_memo_get(key);
    if (m && m->name[0]) {
        strncpy(out, m->name, outn - 1);
        out[outn - 1] = 0;
        return;
    }

    u8 buf[96]; u32 full = 0;
    u32 n = disk_peek(key, buf, sizeof(buf), &full);
    if (n && full < 96) {
        if (blob_is_tag(buf, n)) {
            u32 z = n < outn - 1 ? n : outn - 1;
            memcpy(out, buf, z); out[z] = 0;
            if (m) { strncpy(m->name, out, 79); m->is_tag = 1; }
            return;
        }
        if (blob_is_name(buf, n)) {
            u32 z = n < outn - 1 ? n : outn - 1;
            memcpy(out, buf, z); out[z] = 0;
            if (m) { strncpy(m->name, out, 79); m->is_tag = 0; }
            return;
        }
    }

    /* Prefer a small name child if that child is already on disk. */
    {
        H kids[32]; H k; memcpy(k, key, 32);
        u32 kc = cvm_children(k, kids, 32);
        if (kc > 32) kc = 32;
        for (u32 i = 0; i < kc; i++) {
            if (same_key(kids[i], key)) continue;
            u8 nb[96]; u32 fsz = 0;
            u32 nn = disk_peek(kids[i], nb, sizeof(nb), &fsz);
            if (!nn || fsz >= 96) continue;
            if (!blob_is_name(nb, nn)) continue;
            u32 z = nn < outn - 1 ? nn : outn - 1;
            memcpy(out, nb, z); out[z] = 0;
            if (m) { strncpy(m->name, out, 79); m->is_tag = 0; }
            return;
        }
    }

    snprintf(out, outn, "%02x%02x%02x%02x", key[0], key[1], key[2], key[3]);
    if (m) { strncpy(m->name, out, 79); m->is_tag = 0; }
}

/* Enumerate children of key as explorer rows (skip self-loops).
 * cvm_children uses disk _ch.bin then one network fetch — OK once, cached. */
static u32 tag_child_at(const u8 *parent, u32 row, u8 child_out[32]) {
    H kids[256]; H p; memcpy(p, parent, 32);
    u32 kc = cvm_children(p, kids, 256);
    if (kc > 256) kc = 256;
    u32 r = 0;
    for (u32 i = 0; i < kc; i++) {
        if (zero_key(kids[i]) || same_key(kids[i], parent)) continue;
        if (r == row) {
            memcpy(child_out, kids[i], 32);
            return 1;
        }
        r++;
    }
    return 0;
}

static u32 tag_child_count(const u8 *parent) {
    H kids[256]; H p; memcpy(p, parent, 32);
    u32 kc = cvm_children(p, kids, 256);
    if (kc > 256) kc = 256;
    u32 r = 0;
    for (u32 i = 0; i < kc; i++) {
        if (zero_key(kids[i]) || same_key(kids[i], parent)) continue;
        r++;
        if (r > 256) break;
    }
    return r;
}

/* Open key for a row: block mode uses instr_open_key; tag mode uses child hash. */
static void view_row_open_key(const View *v, u32 row, u8 key_out[32]) {
    memset(key_out, 0, 32);
    if (key_is_tag(v->key)) {
        tag_child_at(v->key, row, key_out);
        return;
    }
    u32 o = block_row_offset(v, row);
    instr_open_key(cvm_cached_base(), cvm_cached_len(), o, key_out);
}

static float tag_row_hit_width(const u8 *child_key) {
    char nm[96];
    key_display_name(child_key, nm, sizeof(nm));
    float w = measure_str(NAME_SIZE, nm) + PAD_X + 24.0f;
    if (key_is_tag(child_key)) w += measure_str(SUM_SIZE, "tag") + NAME_GAP;
    if (w < MIN_HIT_W) w = MIN_HIT_W;
    return w;
}

#endif
