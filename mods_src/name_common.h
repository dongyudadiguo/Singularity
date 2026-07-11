#ifndef NAME_COMMON_H
#define NAME_COMMON_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

/* token + leaf name + path in tag graph */
typedef struct {
    H token;
    char name[96];
    char path[160];
} Entry;

static Entry g_names[4096];
static u32 g_name_n;
static int g_name_loaded;

extern __declspec(dllimport) u32 cvm_children(const H parent, H *out, u32 cap);
/* NOTE: cvm_file_read downloads FULL blobs on the main conn. Do not call it
 * from completion build for unknown children — only for known-small strings
 * when disk cache already misses AND we have a hard size budget. Prefer disk. */

static int str_prefix_ci_us(const char *name, const char *query) {
    if (!query || !*query) return 0;
    const char *a = name, *b = query;
    while (*a && *b) {
        char x = *a++, y = *b++;
        if (x == '_') { b--; continue; }
        if (y == '_') { a--; continue; }
        if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
        if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
        if (x != y) return 0;
    }
    return !*b;
}

static int same32(const u8 *a, const u8 *b) {
    for (int i = 0; i < 32; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

static void cache_bin_path(const H h, char *path, u32 pathn) {
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

static int disk_exists_size(const H h, u32 *full_sz) {
    char path[140];
    cache_bin_path(h, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) { if (full_sz) *full_sz = 0; return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    fclose(f);
    if (sz < 0) return 0;
    if (full_sz) *full_sz = (u32)sz;
    return 1;
}

static u32 disk_peek(const H h, u8 *out, u32 cap, u32 *full_sz) {
    char path[140];
    cache_bin_path(h, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) { if (full_sz) *full_sz = 0; return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); if (full_sz) *full_sz = 0; return 0; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); if (full_sz) *full_sz = 0; return 0; }
    if (full_sz) *full_sz = (u32)sz;
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    u32 got = 0;
    if (out && cap) got = (u32)fread(out, 1, cap, f);
    fclose(f);
    return got;
}

static int is_printable_buf(const u8 *p, u32 n) {
    if (!n || n >= 96) return 0;
    for (u32 i = 0; i < n; i++) if (p[i] < 32 || p[i] > 126) return 0;
    return 1;
}

static int is_tag_buf(const u8 *p, u32 n) {
    return is_printable_buf(p, n) && p[0] == '#';
}

static int is_name_buf(const u8 *p, u32 n) {
    return is_printable_buf(p, n) && p[0] != '#';
}

static void hex4_name(const H tok, char *out, u32 outn) {
    snprintf(out, outn, "%02x%02x%02x%02x", tok[0], tok[1], tok[2], tok[3]);
}

static void path_join(const char *parent, const char *leaf, char *out, u32 outn) {
    if (!leaf || !leaf[0]) { if (outn) out[0] = 0; return; }
    if (!parent || !parent[0]) {
        strncpy(out, leaf, outn - 1);
        out[outn - 1] = 0;
        return;
    }
    snprintf(out, outn, "%s/%s", parent, leaf);
}

static void add_entry(const H tok, const char *name, const char *path) {
    if (g_name_n >= 4096 || zero32(tok)) return;
    for (u32 i = 0; i < g_name_n; i++) if (same32(g_names[i].token, tok)) return;
    memcpy(g_names[g_name_n].token, tok, 32);
    memset(g_names[g_name_n].name, 0, 96);
    memset(g_names[g_name_n].path, 0, 160);
    if (name && name[0]) strncpy(g_names[g_name_n].name, name, 95);
    else hex4_name(tok, g_names[g_name_n].name, 96);
    if (path && path[0]) strncpy(g_names[g_name_n].path, path, 159);
    else strncpy(g_names[g_name_n].path, g_names[g_name_n].name, 159);
    g_name_n++;
}

/* Well-known content-addressed tags (sha256 of their text). Avoids network
 * classification of the main roots that completion/explorer need. */
typedef struct { H key; const char *text; } KnownTag;
static int known_tag_text(const H key, char *out, u32 outn) {
    static const KnownTag k[] = {
        {{0xac,0x79,0x84,0x37,0x38,0x60,0xa5,0x14,0x10,0x56,0x1d,0x1c,0xbc,0x5e,0x1b,0x64,0xa3,0x07,0x27,0x75,0xce,0xb8,0x75,0x66,0xfc,0x23,0x1c,0xf4,0x18,0x0f,0xb9,0x89}, "#TAG"},
        {{0xf8,0x0b,0x67,0xf8,0xe0,0x92,0x47,0x67,0x0f,0xa5,0xe3,0xd5,0x25,0x88,0xc3,0x7c,0x29,0x90,0xbd,0x38,0x4c,0x58,0x6b,0x8d,0x4a,0xc1,0xbe,0xae,0xc7,0x32,0xb3,0xfc}, "#atomic"},
        {{0x1d,0x3e,0xe9,0x75,0xc6,0x69,0xef,0x9c,0x43,0x7a,0x63,0xca,0x3d,0xdc,0x66,0x5f,0xf5,0xe4,0x92,0x1f,0xbd,0x0a,0x5c,0x00,0xec,0x69,0xd4,0x7f,0xc5,0xf7,0x48,0xc4}, "#editor"},
        {{0xa0,0x76,0x5f,0x93,0xca,0x93,0x70,0x49,0xc1,0x0f,0xf4,0x74,0x09,0x5d,0xde,0xed,0x62,0x4d,0x65,0xe0,0x0e,0xd6,0xcb,0xbb,0x12,0xa3,0x4d,0xc3,0x52,0xb3,0x75,0x0c}, "#ui"},
    };
    for (u32 i = 0; i < sizeof(k)/sizeof(k[0]); i++) {
        if (same32(k[i].key, key)) {
            if (out && outn) { strncpy(out, k[i].text, outn - 1); out[outn - 1] = 0; }
            return 1;
        }
    }
    return 0;
}

/* Name child: ONLY disk-cached small blobs. Never network-download DLLs. */
static int name_from_children_disk(const H node, char *out, u32 outn) {
    H kids[32];
    u32 n = cvm_children(node, kids, 32);
    if (n > 32) n = 32;
    for (u32 i = 0; i < n; i++) {
        if (same32(kids[i], node)) continue;
        u32 full = 0;
        u8 buf[96];
        u32 got = disk_peek(kids[i], buf, sizeof(buf) - 1, &full);
        if (!got || full >= 96) continue;
        if (!is_name_buf(buf, got)) continue;
        if (got >= outn) got = outn - 1;
        memcpy(out, buf, got);
        out[got] = 0;
        return 1;
    }
    return 0;
}

/* Also accept name child that is sha256(name) content-addressed and on disk,
 * or computable without download: if child hash equals sha256 of a short name
 * we don't have sha here. Disk-only is enough when install already cached names.
 */

typedef struct {
    H node;
    char path[160];
} QItem;

static void tag_graph_build(void) {
    static const H root = {
        0xac,0x79,0x84,0x37,0x38,0x60,0xa5,0x14,
        0x10,0x56,0x1d,0x1c,0xbc,0x5e,0x1b,0x64,
        0xa3,0x07,0x27,0x75,0xce,0xb8,0x75,0x66,
        0xfc,0x23,0x1c,0xf4,0x18,0x0f,0xb9,0x89
    };

    QItem queue[512];
    u32 qh = 0, qt = 0;
    H seen[4096];
    u32 seen_n = 0;

    memcpy(queue[qt].node, root, 32);
    strncpy(queue[qt].path, "#TAG", 159);
    qt++;
    memcpy(seen[seen_n++], root, 32);

    while (qh < qt && g_name_n < 4096 && seen_n < 4096) {
        H node;
        char parent_path[160];
        memcpy(node, queue[qh].node, 32);
        strncpy(parent_path, queue[qh].path, 159);
        parent_path[159] = 0;
        qh++;

        H kids[256];
        u32 kc = cvm_children(node, kids, 256);
        if (kc > 256) kc = 256;

        for (u32 i = 0; i < kc; i++) {
            if (zero32(kids[i]) || same32(kids[i], node)) continue;

            int already = 0;
            for (u32 s = 0; s < seen_n; s++) {
                if (same32(seen[s], kids[i])) { already = 1; break; }
            }
            if (already) continue;
            if (seen_n < 4096) memcpy(seen[seen_n++], kids[i], 32);

            char nm[96];
            char full[160];
            nm[0] = 0; full[0] = 0;

            /* Known tag roots — recurse without I/O */
            if (known_tag_text(kids[i], nm, sizeof(nm))) {
                path_join(parent_path, nm, full, sizeof(full));
                if (qt < 512) {
                    memcpy(queue[qt].node, kids[i], 32);
                    strncpy(queue[qt].path, full, 159);
                    queue[qt].path[159] = 0;
                    qt++;
                }
                continue;
            }

            /* Disk-cached name child => token candidate (no DLL download). */
            if (name_from_children_disk(kids[i], nm, sizeof(nm))) {
                path_join(parent_path, nm, full, sizeof(full));
                add_entry(kids[i], nm, full);
                continue;
            }

            u32 fullsz = 0;
            u8 buf[96];
            u32 got = disk_peek(kids[i], buf, sizeof(buf), &fullsz);

            if (fullsz >= 96) {
                hex4_name(kids[i], nm, sizeof(nm));
                path_join(parent_path, nm, full, sizeof(full));
                add_entry(kids[i], nm, full);
                continue;
            }

            if (got && fullsz > 0 && fullsz < 96) {
                u32 use = got < fullsz ? got : fullsz;
                if (is_tag_buf(buf, use)) {
                    if (use > 95) use = 95;
                    memcpy(nm, buf, use); nm[use] = 0;
                    path_join(parent_path, nm, full, sizeof(full));
                    if (qt < 512) {
                        memcpy(queue[qt].node, kids[i], 32);
                        strncpy(queue[qt].path, full, 159);
                        queue[qt].path[159] = 0;
                        qt++;
                    }
                } else {
                    if (is_name_buf(buf, use)) {
                        if (use > 95) use = 95;
                        memcpy(nm, buf, use); nm[use] = 0;
                    } else {
                        hex4_name(kids[i], nm, sizeof(nm));
                    }
                    path_join(parent_path, nm, full, sizeof(full));
                    add_entry(kids[i], nm, full);
                }
                continue;
            }

            /* Uncached opaque node: DO NOT network-fetch. Record hex token. */
            hex4_name(kids[i], nm, sizeof(nm));
            path_join(parent_path, nm, full, sizeof(full));
            add_entry(kids[i], nm, full);
        }
    }
}

#define TAG_COMPLETION_CACHE "cache/tag_completion.bin"
#define TAG_COMPLETION_MAGIC 0x32474154u /* TAG2 */

static int name_load_tag_cache(void) {
    FILE *f = fopen(TAG_COMPLETION_CACHE, "rb");
    if (!f) return 0;
    u32 magic = 0, n = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != TAG_COMPLETION_MAGIC) { fclose(f); return 0; }
    if (fread(&n, 4, 1, f) != 1 || n == 0 || n > 4096) { fclose(f); return 0; }
    g_name_n = (u32)fread(g_names, sizeof(Entry), n, f);
    fclose(f);
    return g_name_n > 0;
}

static void name_save_tag_cache(void) {
    if (g_name_n == 0) return;
    FILE *f = fopen(TAG_COMPLETION_CACHE, "wb");
    if (!f) return;
    u32 magic = TAG_COMPLETION_MAGIC;
    fwrite(&magic, 4, 1, f);
    fwrite(&g_name_n, 4, 1, f);
    fwrite(g_names, sizeof(Entry), g_name_n, f);
    fclose(f);
}

static void name_load_local_fallback(void) {
    const char *paths[] = {
        "instruction_names.bin", "./instruction_names.bin", ".\\instruction_names.bin", 0
    };
    for (int p = 0; paths[p]; p++) {
        FILE *f = fopen(paths[p], "rb");
        if (!f) continue;
        u32 n = 0;
        fread(&n, 4, 1, f);
        if (n > 4096) n = 4096;
        for (u32 i = 0; i < n; i++) {
            Entry e;
            memset(&e, 0, sizeof(e));
            if (fread(e.token, 1, 32, f) != 32) break;
            if (fread(e.name, 1, 96, f) != 96) break;
            strncpy(e.path, e.name, 159);
            if (g_name_n < 4096) g_names[g_name_n++] = e;
        }
        fclose(f);
        return;
    }
}

static void name_load(void) {
    if (g_name_loaded) return;
    g_name_loaded = 1;
    g_name_n = 0;
    if (name_load_tag_cache()) return;
    /* Prefer offline local names first so HUD never blocks on cold graph walk
     * if children cache is missing. Then optionally merge tag graph. */
    name_load_local_fallback();
    if (g_name_n > 0) {
        /* still try graph for paths, but only disk/children — no big downloads */
        u32 before = g_name_n;
        tag_graph_build();
        (void)before;
        name_save_tag_cache();
        return;
    }
    tag_graph_build();
    if (g_name_n > 0) name_save_tag_cache();
}

static void name_path_of(const H token, char *out, u32 outn) {
    if (!outn) return;
    out[0] = 0;
    int nz = 0; for (int i = 0; i < 32; i++) nz |= token[i];
    if (!nz) return;
    name_load();
    for (u32 i = 0; i < g_name_n; i++) {
        if (same32(g_names[i].token, token)) {
            const char *p = g_names[i].path[0] ? g_names[i].path : g_names[i].name;
            strncpy(out, p, outn - 1);
            out[outn - 1] = 0;
            return;
        }
    }
    snprintf(out, outn, "%02x%02x%02x%02x", token[0], token[1], token[2], token[3]);
}
#endif
