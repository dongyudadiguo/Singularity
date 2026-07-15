#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 id_len, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);

#include "../dxgfx.h"

/* Compact multi-view table stored in a single VM var.
 *
 * Layout:
 *   u32 view_count
 *   u32 active
 *   i32 dragging   (-1 none)
 *   u32 _pad
 *   View views[MAX]
 *
 * View:
 *   key[32]
 *   f32 x, y
 *   i32 parent     (-1 none)
 *   i32 linked     (0/1)
 *   f32 link_x, link_y
 *   u32 used
 *   u32 cursor     (row index into block)
 *   u32 _pad0,_pad1
 *
 * Payload:
 *   var_id[32] + u32 op [+ optional fixed args]
 *
 * Hit testing uses measured text width of the token name (+ trailing
 * payload summary) so click targets match the rendered label, not a
 * fixed 520px column.
 *
 * Ops:
 *  0 init        args: key[32], f32 x, f32 y
 *  1 count       -> u32
 *  2 active      -> u32
 *  3 set_active  args: u32 index
 *  4 get_xy      args: u32 index -> f32 x, f32 y
 *  5 set_xy      args: u32 index, f32 x, f32 y
 *  6 get_key     args: u32 index -> key[32]
 *  7 get_parent  args: u32 index -> i32
 *  8 get_link    args: u32 index -> f32 lx, f32 ly, i32 linked
 *  9 get_cursor  args: u32 index -> u32
 * 10 set_cursor  args: u32 index, u32 cursor
 * 11 drag_begin  args: u32 index
 * 12 drag_step   args: f32 dx, f32 dy   (world delta already /zoom)
 * 13 drag_end
 * 14 open        args: key[32], f32 x, f32 y, i32 parent, f32 lx, f32 ly
 * 15 hit_title   stack: f32 mx,my  args: f32 title_h [,f32 pad_w unused]
 * 16 hit_row     stack: f32 mx,my  args: f32 row_h [,f32 pad_w unused], u32 row_count
 * 17 move_by     args: u32 index, f32 dx, f32 dy
 * 18 ensure      args: key[32], f32 x, f32 y
 * 19 get_dragging -> i32
 * 20 set_cursor_active args: u32
 * 21 active_cursor -> u32
 * 22 active_key -> key[32]
 * 23 cursor_add  args: i32 delta
 * 24 cursor_dec
 * 29 pointer_lmb stack mx,my ; text-width hit
 * 30 pointer_rmb stack mx,my ; text-width hit + open
 * 32 get_drag_xy -> f32 x,y of dragging view
 * 33 set_drag_xy_stack absolute
 */

#define VIEW_MAX 32
#define VIEW_SIZE 72
#define HEADER_SIZE 16
#define BLOB_SIZE (HEADER_SIZE + VIEW_MAX * VIEW_SIZE)
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

/* Tokens whose 32-byte payload is itself a content/logical hash to open.
 * Ordinary natives (views, var_*, etc.) may also have 32-byte payloads that
 * are NOT open targets — open the row token itself for those.
 * Match by instruction name (not hardcoded DLL content hash — those drift). */
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

/* Width of one rendered instruction row at world layout units.
 * Must stay >= views_render layout: swatch + name + icon + summary.
 */
static float row_text_width(const u8 *instr) {
    const char *nm = token_name(instr);
    float icon = NAME_SIZE; /* matches dxgfx_icon_size roughly */
    float w = 4.0f + 4.0f; /* swatch + pad */
    /* var_*_payload specialized rows hide the name and show icon+id+size */
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
        else w += 80.0f; /* id hex estimate */
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

/* Resolve row index into instruction pointer; returns 0 if past end. */
static const u8 *row_instr(const View *v, u32 row, u32 *out_count) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0;
    u32 r = 0;
    while (o + 36 <= nlen && !zero_key(b + o)) {
        u32 pn = *(u32 *)(b + o + 32);
        if (o + 36 + pn > nlen) break;
        if (r == row) {
            if (out_count) *out_count = r + 1; /* not total; caller may ignore */
            return b + o;
        }
        o += 36 + pn;
        r++;
        if (r > 256) break;
    }
    if (out_count) *out_count = r;
    return 0; /* past last instruction: the <end> row */
}

static u32 block_row_count(const View *v) {
    H h;
    cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 nlen = cvm_cached_len();
    u32 o = 0;
    u32 r = 0;
    while (o + 36 <= nlen && !zero_key(b + o)) {
        u32 pn = *(u32 *)(b + o + 32);
        if (o + 36 + pn > nlen) break;
        o += 36 + pn;
        r++;
        if (r > 256) break;
    }
    return r; /* number of real instructions; <end> is at index r */
}

static float row_hit_width(const View *v, int row) {
    if (row < 0) return MIN_HIT_W;
    const u8 *instr = row_instr(v, (u32)row, 0);
    if (!instr) {
        /* <end> sentinel */
        float w = measure_str(16.0f, "<end>") + PAD_X;
        if (w < MIN_HIT_W) w = MIN_HIT_W;
        return w;
    }
    return row_text_width(instr);
}

static u8 *blob(const H id, u32 *size_out) {
    u32 size = 0;
    u8 *p = cvm_var_get(id, 32, &size);
    if (!p || size < (u32)sizeof(Table)) return 0;
    if (size_out) *size_out = size;
    return p;
}

static Table *load(const H id) {
    return (Table *)blob(id, 0);
}

static void store(const H id, Table *t) {
    cvm_var_write(id, 32, (const u8 *)t, (u32)sizeof(Table));
}

__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 36) { cont(); return; }
    H id;
    memcpy(id, p, 32);
    u32 op = *(u32 *)(p + 32);
    const u8 *args = p + 36;
    u32 an = n - 36;

    Table *tp = load(id);
    if (!tp) {
        if (op == 0 || op == 18) {
            /* create empty table then continue into op handler */
            cvm_var_set(id, 32, (u32)sizeof(Table));
            Table empty;
            memset(&empty, 0, sizeof(empty));
            empty.dragging = -1;
            cvm_var_write(id, 32, (const u8 *)&empty, (u32)sizeof(Table));
            tp = load(id);
            if (!tp) { cont(); return; }
        } else {
            if (op == 1) { u32 z = 0; memcpy(slot(4), &z, 4); }
            else if (op == 2) { u32 z = 0; memcpy(slot(4), &z, 4); }
            else if (op == 14) { u32 z = 0xffffffffu; memcpy(slot(4), &z, 4); }
            else if (op == 15) { int z = -1; (void)from(8); memcpy(slot(4), &z, 4); }
            else if (op == 16) { int z = -1; (void)from(8); memcpy(slot(4), &z, 4); memcpy(slot(4), &z, 4); }
            else if (op == 19) { int z = -1; memcpy(slot(4), &z, 4); }
            else if (op == 21) { u32 z = 0; memcpy(slot(4), &z, 4); }
            else if (op == 22) { u8 z[32]; memset(z, 0, 32); memcpy(slot(32), z, 32); }
            else if (op == 29 || op == 30) { u32 z = 0; (void)from(8); memcpy(slot(4), &z, 4); }
            else if (op == 32) { float z = 0.0f; memcpy(slot(4), &z, 4); memcpy(slot(4), &z, 4); }
            cont();
            return;
        }
    }
    Table t = *tp;

    if (op == 0) {
        /* init: create/resize and seed view0 */
        if (an < 40) { cont(); return; }
        cvm_var_set(id, 32, (u32)sizeof(Table));
        memset(&t, 0, sizeof(t));
        t.dragging = -1;
        t.count = 1;
        t.active = 0;
        zero_view(&t.views[0]);
        t.views[0].used = 1;
        memcpy(t.views[0].key, args, 32);
        t.views[0].x = *(float *)(args + 32);
        t.views[0].y = *(float *)(args + 36);
        store(id, &t);
    } else if (op == 1) {
        memcpy(slot(4), &t.count, 4);
    } else if (op == 2) {
        memcpy(slot(4), &t.active, 4);
    } else if (op == 3) {
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) t.active = idx;
        }
        store(id, &t);
    } else if (op == 4) {
        float x = 0.0f, y = 0.0f;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                x = t.views[idx].x; y = t.views[idx].y;
            }
        }
        memcpy(slot(4), &x, 4); memcpy(slot(4), &y, 4);
    } else if (op == 5) {
        if (an >= 12) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                t.views[idx].x = *(float *)(args + 4);
                t.views[idx].y = *(float *)(args + 8);
            }
        }
        store(id, &t);
    } else if (op == 6) {
        u8 key[32]; memset(key, 0, 32);
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) memcpy(key, t.views[idx].key, 32);
        }
        memcpy(slot(32), key, 32);
    } else if (op == 7) {
        int parent = -1;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) parent = t.views[idx].parent;
        }
        memcpy(slot(4), &parent, 4);
    } else if (op == 8) {
        float lx = 0.0f, ly = 0.0f; int linked = 0;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                lx = t.views[idx].link_x; ly = t.views[idx].link_y;
                linked = t.views[idx].linked;
            }
        }
        memcpy(slot(4), &lx, 4); memcpy(slot(4), &ly, 4); memcpy(slot(4), &linked, 4);
    } else if (op == 9) {
        u32 cur = 0;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) cur = t.views[idx].cursor;
        }
        memcpy(slot(4), &cur, 4);
    } else if (op == 10) {
        if (an >= 8) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) t.views[idx].cursor = *(u32 *)(args + 4);
        }
        store(id, &t);
    } else if (op == 11) {
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) t.dragging = (int)idx;
        }
        store(id, &t);
    } else if (op == 12) {
        if (an >= 8 && t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
            t.views[t.dragging].x += *(float *)args;
            t.views[t.dragging].y += *(float *)(args + 4);
            store(id, &t);
        }
    } else if (op == 13) {
        t.dragging = -1;
        store(id, &t);
    } else if (op == 14) {
        u32 out = 0xffffffffu;
        if (an >= 52) {
            const u8 *key = args;
            float x = *(float *)(args + 32);
            float y = *(float *)(args + 36);
            int parent = *(int *)(args + 40);
            float lx = *(float *)(args + 44);
            float ly = *(float *)(args + 48);
            int found = -1;
            for (u32 i = 0; i < t.count; i++) {
                if (t.views[i].used && same_key(t.views[i].key, key)) { found = (int)i; break; }
            }
            if (found >= 0) {
                t.active = (u32)found;
                t.dragging = found;
                out = (u32)found;
            } else if (t.count < VIEW_MAX) {
                u32 i = t.count++;
                zero_view(&t.views[i]);
                t.views[i].used = 1;
                memcpy(t.views[i].key, key, 32);
                t.views[i].x = x;
                t.views[i].y = y;
                t.views[i].parent = parent;
                t.views[i].linked = 1;
                t.views[i].link_x = lx;
                t.views[i].link_y = ly;
                t.active = i;
                t.dragging = (int)i;
                out = i;
            }
        }
        store(id, &t);
        memcpy(slot(4), &out, 4);
    } else if (op == 15) {
        /* hit title bar: text-width */
        float my = *(float *)from(4);
        float mx = *(float *)from(4);
        float title_h = 32.0f;
        if (an >= 4) title_h = *(float *)args;
        int hit = -1;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float width = title_text_width((u32)i, v);
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                hit = i; break;
            }
        }
        memcpy(slot(4), &hit, 4);
    } else if (op == 16) {
        /* hit row: text-width per row */
        float my = *(float *)from(4);
        float mx = *(float *)from(4);
        float row_h = 24.0f; u32 row_count = 64;
        if (an >= 4) row_h = *(float *)args;
        if (an >= 12) row_count = *(u32 *)(args + 8);
        else if (an >= 8) row_count = *(u32 *)(args + 4);
        int vhit = -1, rhit = -1;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float rel = my - v->y;
            if (rel < 0.0f) continue;
            int row = (int)(rel / row_h);
            if (row < 0 || (u32)row >= row_count) continue;
            float width = row_hit_width(v, row);
            if (mx < v->x || mx >= v->x + width) continue;
            vhit = i; rhit = row; break;
        }
        memcpy(slot(4), &vhit, 4); memcpy(slot(4), &rhit, 4);
    } else if (op == 17) {
        if (an >= 12) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                t.views[idx].x += *(float *)(args + 4);
                t.views[idx].y += *(float *)(args + 8);
            }
        }
        store(id, &t);
    } else if (op == 18) {
        /* ensure seeded */
        if (an >= 40 && t.count == 0) {
            t.dragging = -1;
            t.count = 1;
            t.active = 0;
            zero_view(&t.views[0]);
            t.views[0].used = 1;
            memcpy(t.views[0].key, args, 32);
            t.views[0].x = *(float *)(args + 32);
            t.views[0].y = *(float *)(args + 36);
            store(id, &t);
        }
    } else if (op == 19) {
        memcpy(slot(4), &t.dragging, 4);
    } else if (op == 20) {
        if (an >= 4 && t.active < t.count && t.views[t.active].used) {
            t.views[t.active].cursor = *(u32 *)args;
            store(id, &t);
        }
    } else if (op == 21) {
        u32 cur = 0;
        if (t.active < t.count && t.views[t.active].used) cur = t.views[t.active].cursor;
        memcpy(slot(4), &cur, 4);
    } else if (op == 22) {
        u8 key[32]; memset(key, 0, 32);
        if (t.active < t.count && t.views[t.active].used) memcpy(key, t.views[t.active].key, 32);
        memcpy(slot(32), key, 32);
    } else if (op == 23) {
        /* cursor_add: args i32 delta on active, clamped to block rows + end */
        if (an >= 4 && t.active < t.count && t.views[t.active].used) {
            int d = *(int *)args;
            int cur = (int)t.views[t.active].cursor + d;
            u32 maxr = block_row_count(&t.views[t.active]); /* end index == count */
            if (cur < 0) cur = 0;
            if ((u32)cur > maxr) cur = (int)maxr;
            t.views[t.active].cursor = (u32)cur;
            store(id, &t);
        }
    } else if (op == 24) {
        /* cursor_dec by 1 */
        if (t.active < t.count && t.views[t.active].used) {
            if (t.views[t.active].cursor > 0) t.views[t.active].cursor--;
            store(id, &t);
        }
    } else if (op == 29) {
        /* pointer_lmb: text-width title / row hit */
        float my = *(float *)from(4);
        float mx = *(float *)from(4);
        float title_h = 32.0f, row_h = 24.0f;
        u32 row_count = 256;
        if (an >= 4) title_h = *(float *)args;
        if (an >= 12) row_h = *(float *)(args + 8);
        else if (an >= 8) row_h = *(float *)(args + 4);
        if (an >= 16) row_count = *(u32 *)(args + 12);
        else if (an >= 12) row_count = *(u32 *)(args + 8);
        int handled = 0;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float width = title_text_width((u32)i, v);
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                t.active = (u32)i;
                handled = 1;
                break;
            }
        }
        if (!handled) {
            for (int i = (int)t.count - 1; i >= 0; i--) {
                View *v = &t.views[i];
                if (!v->used) continue;
                float rel = my - v->y;
                if (rel < 0.0f) continue;
                int row = (int)(rel / row_h);
                if (row < 0 || (u32)row > row_count) continue;
                float width = row_hit_width(v, row);
                if (mx < v->x || mx >= v->x + width) continue;
                t.active = (u32)i;
                t.views[i].cursor = (u32)row;
                handled = 1;
                break;
            }
        }
        store(id, &t);
        memcpy(slot(4), &handled, 4);
    } else if (op == 30) {
        /* pointer_rmb: title drag / row open with text-width hit */
        float my = *(float *)from(4);
        float mx = *(float *)from(4);
        float title_h = 32.0f, row_h = 24.0f;
        u32 row_count = 256;
        if (an >= 4) title_h = *(float *)args;
        if (an >= 12) row_h = *(float *)(args + 8);
        else if (an >= 8) row_h = *(float *)(args + 4);
        if (an >= 16) row_count = *(u32 *)(args + 12);
        else if (an >= 12) row_count = *(u32 *)(args + 8);
        u32 handled = 0;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float width = title_text_width((u32)i, v);
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                t.active = (u32)i;
                t.dragging = i;
                handled = 1;
                store(id, &t);
                memcpy(slot(4), &handled, 4);
                cont();
                return;
            }
        }
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            float rel = my - v->y;
            if (rel < 0.0f) continue;
            int row = (int)(rel / row_h);
            if (row < 0 || (u32)row >= row_count) continue;
            float width = row_hit_width(v, row);
            if (mx < v->x || mx >= v->x + width) continue;
            H h;
            cvm_resolve_payload_hash(v->key, h);
            u8 *b = cvm_cached_base();
            u32 nlen = cvm_cached_len();
            u32 o = 0;
            for (u32 r = 0; r < (u32)row && o + 36 <= nlen; r++) {
                if (zero_key(b + o)) { o = nlen; break; }
                u32 pn = *(u32 *)(b + o + 32);
                if (o + 36 + pn > nlen) { o = nlen; break; }
                o += 36 + pn;
            }
            u8 key[32];
            memset(key, 0, 32);
            if (o + 32 <= nlen && !zero_key(b + o)) memcpy(key, b + o, 32);
            if (zero_key(key)) break;
            /* Only hash-carrier tokens open their 32-byte payload hash.
             * Specialized natives (e.g. views) open as themselves so their
             * surface override / firstchild definition block is visible. */
            if (o + 36 <= nlen) {
                u32 pn = *(u32 *)(b + o + 32);
                if (pn == 32 && o + 68 <= nlen && is_hash_carrier(key)) {
                    u8 ph[32];
                    memcpy(ph, b + o + 36, 32);
                    if (!zero_key(ph)) memcpy(key, ph, 32);
                }
            }
            int found = -1;
            for (u32 j = 0; j < t.count; j++) {
                if (t.views[j].used && same_key(t.views[j].key, key)) { found = (int)j; break; }
            }
            if (found >= 0) {
                t.active = (u32)found;
                t.dragging = found;
                t.views[found].x = mx;
                t.views[found].y = my;
            } else if (t.count < VIEW_MAX) {
                u32 ni = t.count++;
                zero_view(&t.views[ni]);
                t.views[ni].used = 1;
                memcpy(t.views[ni].key, key, 32);
                t.views[ni].x = mx;
                t.views[ni].y = my;
                t.views[ni].parent = i;
                t.views[ni].linked = 1;
                /* link origin at measured mid of the row text */
                t.views[ni].link_x = width * 0.5f;
                if (t.views[ni].link_x < 24.0f) t.views[ni].link_x = 24.0f;
                t.views[ni].link_y = (float)row * row_h + 10.0f;
                t.active = ni;
                t.dragging = (int)ni;
            }
            handled = 1;
            break;
        }
        store(id, &t);
        memcpy(slot(4), &handled, 4);
    }

    if (op == 32) {
        float x = 0.0f, y = 0.0f;
        if (t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
            x = t.views[t.dragging].x;
            y = t.views[t.dragging].y;
        }
        memcpy(slot(4), &x, 4); memcpy(slot(4), &y, 4);
    } else if (op == 33) {
        float y = *(float *)from(4);
        float x = *(float *)from(4);
        if (t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
            t.views[t.dragging].x = x;
            t.views[t.dragging].y = y;
            store(id, &t);
        }
    }
    cont();
}
