#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);

#define MAX_BLOCK (1u << 20)
#define MAX_ID 256

/* payload: typein_var_id (any size — entire payload is id)
 * stack: u32 byte_offset of instruction in currently selected block
 *
 * If the instruction token is a var_*_payload family member, rebuild its
 * payload from the typein C-string:
 *   var_read_payload:  text -> id bytes
 *   var_write_payload: text -> id (preserve trailing data if any under new header)
 *   var_set_payload:
 *      "<id>"            -> keep size (or 0)
 *      "<id> <size>"     -> set id + size
 *      "<size>"          -> digits only: keep id, change size
 *
 * Token match is by comparing against known hashes from instruction_names if
 * present; also accepts legacy layouts.
 */

typedef struct { H token; char name[96]; } Entry;
static Entry g_ent[512];
static u32 g_n;
static int g_loaded;
static H t_set, t_read, t_write;
static int t_ok;

static void load_names(void) {
    if (g_loaded) return;
    g_loaded = 1;
    FILE *f = fopen("instruction_names.bin", "rb");
    if (!f) f = fopen("./instruction_names.bin", "rb");
    if (!f) return;
    u32 n = 0; fread(&n, 4, 1, f);
    if (n > 512) n = 512;
    g_n = (u32)fread(g_ent, sizeof(Entry), n, f);
    fclose(f);
    for (u32 i = 0; i < g_n; i++) {
        if (!strcmp(g_ent[i].name, "var_set_payload")) { memcpy(t_set, g_ent[i].token, 32); t_ok |= 1; }
        if (!strcmp(g_ent[i].name, "var_read_payload")) { memcpy(t_read, g_ent[i].token, 32); t_ok |= 2; }
        if (!strcmp(g_ent[i].name, "var_write_payload")) { memcpy(t_write, g_ent[i].token, 32); t_ok |= 4; }
    }
}

static int same32(const u8 *a, const u8 *b) { return !memcmp(a, b, 32); }

static int is_digits(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) if (*p < '0' || *p > '9') return 0;
    return 1;
}

static u32 parse_u32(const char *s) {
    u32 v = 0;
    for (const char *p = s; *p >= '0' && *p <= '9'; p++) v = v * 10u + (u32)(*p - '0');
    return v;
}

/* Replace payload at instruction offset; token unchanged. */
static int replace_payload(u32 off, const u8 *np, u32 nn) {
    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (off + 36 > l) return 0;
    u32 old = *(u32 *)(b + off + 32);
    if (off + 36 + old > l) return 0;
    if (l - old + nn > MAX_BLOCK) return 0;
    memmove(b + off + 36 + nn, b + off + 36 + old, l - (off + 36 + old));
    *(u32 *)(b + off + 32) = nn;
    if (nn) memcpy(b + off + 36, np, nn);
    cvm_cached_set_len(l - old + nn);
    return 1;
}

static void trim(char *s) {
    u32 n = (u32)strlen(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = 0;
    u32 i = 0; while (s[i] == ' ' || s[i] == '\t') i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

__declspec(dllexport) void run(void) {
    u32 off = *(u32 *)pop(4);
    u8 *vid = cvm_payload();
    u32 vid_len = cvm_payload_size();
    if (!vid_len) { cont(); return; }

    u32 tsize = 0;
    u8 *text = cvm_var_get(vid, vid_len, &tsize);
    if (!text || !tsize) { cont(); return; }

    char buf[512];
    u32 z = 0;
    while (z < tsize && text[z] && z < sizeof(buf) - 1) { buf[z] = (char)text[z]; z++; }
    buf[z] = 0;
    trim(buf);
    if (!buf[0]) { cont(); return; }

    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (off + 36 > l) { cont(); return; }
    u8 tok[32]; memcpy(tok, b + off, 32);
    u32 pn = *(u32 *)(b + off + 32);
    if (off + 36 + pn > l) { cont(); return; }
    const u8 *oldp = b + off + 36;

    load_names();
    if (!t_ok) { cont(); return; }

    u8 neu[4 + MAX_ID + 4 + 256];
    u32 nn = 0;

    if (same32(tok, t_read)) {
        /* entire payload = id bytes from text */
        u32 id_len = (u32)strlen(buf);
        if (id_len > MAX_ID) id_len = MAX_ID;
        memcpy(neu, buf, id_len);
        nn = id_len;
        replace_payload(off, neu, nn);
        cont(); return;
    }

    if (same32(tok, t_write)) {
        /* id_len + id [+ preserve old data tail if new-format] */
        u32 id_len = (u32)strlen(buf);
        if (id_len > MAX_ID) id_len = MAX_ID;
        u32 old_id_len = 0, old_rest = 0;
        const u8 *old_data = 0;
        if (pn >= 4) {
            u32 il = *(u32 *)oldp;
            if (il > 0 && il <= MAX_ID && pn >= 4 + il) {
                old_id_len = il;
                old_rest = pn - 4 - il;
                old_data = oldp + 4 + il;
            } else if (pn >= 32) {
                old_rest = pn > 32 ? pn - 32 : 0;
                old_data = oldp + 32;
            }
        }
        *(u32 *)neu = id_len;
        memcpy(neu + 4, buf, id_len);
        nn = 4 + id_len;
        if (old_rest && old_data) {
            if (nn + old_rest > sizeof(neu)) old_rest = sizeof(neu) - nn;
            memcpy(neu + nn, old_data, old_rest);
            nn += old_rest;
        }
        replace_payload(off, neu, nn);
        cont(); return;
    }

    if (same32(tok, t_set)) {
        /* determine current id/size from old payload */
        u8 cur_id[MAX_ID];
        u32 cur_id_len = 0;
        u32 cur_size = 0;
        int had = 0;
        if (pn >= 4) {
            u32 il = *(u32 *)oldp;
            if (il > 0 && il <= MAX_ID && pn >= 4 + il) {
                cur_id_len = il;
                memcpy(cur_id, oldp + 4, il);
                u32 rest = pn - 4 - il;
                if (rest == 4) cur_size = *(u32 *)(oldp + 4 + il);
                else cur_size = rest;
                had = 1;
            } else if (pn == 36) {
                cur_id_len = 32; memcpy(cur_id, oldp, 32);
                cur_size = *(u32 *)(oldp + 32); had = 1;
            } else if (pn >= 32) {
                cur_id_len = 32; memcpy(cur_id, oldp, 32);
                cur_size = pn - 32; had = 1;
            }
        }
        (void)had;

        char idpart[300];
        u32 new_size = cur_size;
        int size_only = 0;

        if (is_digits(buf)) {
            /* digits only: change size, keep id */
            new_size = parse_u32(buf);
            size_only = 1;
            if (cur_id_len) { memcpy(idpart, cur_id, cur_id_len); idpart[cur_id_len] = 0; }
            else { idpart[0] = 0; }
        } else {
            /* split last space-separated token if digits -> size */
            char *sp = strrchr(buf, ' ');
            if (sp && sp > buf && is_digits(sp + 1)) {
                *sp = 0;
                trim(buf);
                new_size = parse_u32(sp + 1);
            }
            strncpy(idpart, buf, sizeof(idpart) - 1);
            idpart[sizeof(idpart) - 1] = 0;
        }

        u32 id_len = (u32)strlen(idpart);
        if (!id_len && cur_id_len) {
            memcpy(idpart, cur_id, cur_id_len);
            idpart[cur_id_len] = 0;
            id_len = cur_id_len;
        }
        if (id_len > MAX_ID) id_len = MAX_ID;
        if (!id_len) { cont(); return; }

        *(u32 *)neu = id_len;
        memcpy(neu + 4, idpart, id_len);
        *(u32 *)(neu + 4 + id_len) = new_size;
        nn = 4 + id_len + 4;
        replace_payload(off, neu, nn);
        cont(); return;
    }

    cont();
}
