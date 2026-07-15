#include "block_layout.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);

#define MAX_BLOCK (1u << 20)
#define MAX_ID 256

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
static void trim(char *s) {
    u32 n = (u32)strlen(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = 0;
    u32 i = 0; while (s[i] == ' ' || s[i] == '\t') i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

/* Replace payload at instruction offset; token unchanged. New layout. */
static int replace_payload(u32 off, const u8 *np, u32 nn) {
    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (!bl_ok(b, l, off) || bl_is_end(b + off)) return 0;
    u32 tlen = bl_tlen(b + off);
    u32 old = bl_plen(b + off);
    u32 old_sz = bl_instr_size(b + off);
    u32 new_sz = 8 + tlen + nn;
    if (l - old_sz + new_sz > MAX_BLOCK) return 0;
    /* move tail */
    memmove(b + off + new_sz, b + off + old_sz, l - (off + old_sz));
    /* rewrite plen + payload (token stays) */
    *(u32 *)(b + off + 4 + tlen) = nn;
    if (nn) memcpy(b + off + 8 + tlen, np, nn);
    cvm_cached_set_len(l - old_sz + new_sz);
    (void)old;
    return 1;
}

__declspec(dllexport) void run(void) {
    u32 off = *(u32 *)from(4);
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

    load_names();
    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (!bl_ok(b, l, off) || bl_is_end(b + off)) { cont(); return; }
    u32 tlen = bl_tlen(b + off);
    if (tlen != 32) { cont(); return; }
    u8 tok[32]; memcpy(tok, bl_token_c(b + off), 32);
    u32 pn = bl_plen(b + off);
    const u8 *oldp = bl_payload_c(b + off);

    int kind = 0; /* 1=set 2=read 3=write */
    if ((t_ok & 1) && same32(tok, t_set)) kind = 1;
    else if ((t_ok & 2) && same32(tok, t_read)) kind = 2;
    else if ((t_ok & 4) && same32(tok, t_write)) kind = 3;
    else {
        /* fallback by name table */
        for (u32 i = 0; i < g_n; i++) {
            if (same32(tok, g_ent[i].token)) {
                if (!strcmp(g_ent[i].name, "var_set_payload")) kind = 1;
                else if (!strcmp(g_ent[i].name, "var_read_payload")) kind = 2;
                else if (!strcmp(g_ent[i].name, "var_write_payload")) kind = 3;
                break;
            }
        }
    }
    if (!kind) { cont(); return; }

    u8 npay[512];
    u32 nn = 0;

    if (kind == 2) {
        /* var_read_payload: entire payload is id bytes */
        u32 idn = (u32)strlen(buf);
        if (idn > MAX_ID) idn = MAX_ID;
        memcpy(npay, buf, idn);
        nn = idn;
    } else if (kind == 3) {
        /* var_write_payload: id_len + id [+ keep trailing data if any] */
        u32 idn = (u32)strlen(buf);
        if (idn > MAX_ID) idn = MAX_ID;
        u32 rest = 0;
        const u8 *restp = 0;
        if (pn >= 4) {
            u32 old_id_len = *(u32 *)oldp;
            if (old_id_len > 0 && old_id_len <= 256 && pn >= 4 + old_id_len) {
                rest = pn - 4 - old_id_len;
                restp = oldp + 4 + old_id_len;
            }
        }
        *(u32 *)npay = idn;
        memcpy(npay + 4, buf, idn);
        if (rest && restp && 4 + idn + rest <= sizeof(npay)) {
            memcpy(npay + 4 + idn, restp, rest);
            nn = 4 + idn + rest;
        } else {
            nn = 4 + idn;
        }
    } else {
        /* var_set_payload */
        char idbuf[256]; u32 size_val = 0; int has_size = 0;
        if (is_digits(buf)) {
            /* digits only: keep old id, change size */
            has_size = 1;
            size_val = parse_u32(buf);
            idbuf[0] = 0;
            if (pn >= 4) {
                u32 old_id_len = *(u32 *)oldp;
                if (old_id_len > 0 && old_id_len <= 256 && pn >= 4 + old_id_len) {
                    u32 z2 = old_id_len < 255 ? old_id_len : 255;
                    memcpy(idbuf, oldp + 4, z2); idbuf[z2] = 0;
                }
            }
            if (!idbuf[0]) { cont(); return; }
        } else {
            /* "id" or "id size" */
            char *sp = strchr(buf, ' ');
            if (sp) {
                *sp = 0;
                char *rest = sp + 1;
                while (*rest == ' ') rest++;
                if (is_digits(rest)) { has_size = 1; size_val = parse_u32(rest); }
            }
            strncpy(idbuf, buf, 255); idbuf[255] = 0;
        }
        u32 idn = (u32)strlen(idbuf);
        if (!idn) { cont(); return; }
        *(u32 *)npay = idn;
        memcpy(npay + 4, idbuf, idn);
        if (has_size) {
            *(u32 *)(npay + 4 + idn) = size_val;
            nn = 4 + idn + 4;
        } else if (pn >= 4) {
            u32 old_id_len = *(u32 *)oldp;
            u32 rest = 0;
            if (old_id_len > 0 && old_id_len <= 256 && pn >= 4 + old_id_len)
                rest = pn - 4 - old_id_len;
            if (rest == 4) {
                memcpy(npay + 4 + idn, oldp + 4 + old_id_len, 4);
                nn = 4 + idn + 4;
            } else if (rest) {
                if (4 + idn + rest > sizeof(npay)) rest = sizeof(npay) - 4 - idn;
                memcpy(npay + 4 + idn, oldp + 4 + old_id_len, rest);
                nn = 4 + idn + rest;
            } else {
                nn = 4 + idn;
            }
        } else {
            nn = 4 + idn;
        }
    }

    replace_payload(off, npay, nn);
    cont();
}
