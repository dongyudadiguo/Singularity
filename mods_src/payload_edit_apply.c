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
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);

#define MAX_BLOCK (1u << 20)

typedef struct { H token; char name[96]; } Entry;
static Entry g_ent[1024];
static u32 g_n;
static int g_loaded;

static void load_names(void) {
    if (g_loaded) return;
    g_loaded = 1;
    FILE *f = fopen("instruction_names.bin", "rb");
    if (!f) f = fopen("./instruction_names.bin", "rb");
    if (!f) return;
    u32 n = 0; fread(&n, 4, 1, f);
    if (n > 1024) n = 1024;
    g_n = (u32)fread(g_ent, sizeof(Entry), n, f);
    fclose(f);
}

static const char *tok_name(const u8 *tok) {
    load_names();
    for (u32 i = 0; i < g_n; i++)
        if (!memcmp(g_ent[i].token, tok, 32)) return g_ent[i].name;
    return "";
}

static int same32(const u8 *a, const u8 *b) { return !memcmp(a, b, 32); }

static int replace_payload(u32 off, const u8 *np, u32 nn) {
    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (!bl_ok(b, l, off) || bl_is_end(b + off)) return 0;
    u32 tlen = bl_tlen(b + off);
    u32 old_sz = bl_instr_size(b + off);
    u32 new_sz = 8 + tlen + nn;
    if (l - old_sz + new_sz > MAX_BLOCK) return 0;
    memmove(b + off + new_sz, b + off + old_sz, l - (off + old_sz));
    *(u32 *)(b + off + 4 + tlen) = nn;
    if (nn) memcpy(b + off + 8 + tlen, np, nn);
    cvm_cached_set_len(l - old_sz + new_sz);
    return 1;
}

static void trim(char *s) {
    u32 n = (u32)strlen(s);
    while (n && (s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n')) s[--n]=0;
    u32 i=0; while (s[i]==' '||s[i]=='\t') i++;
    if (i) memmove(s, s+i, strlen(s+i)+1);
}

static int hex_nibble(char c) {
    if (c>='0'&&c<='9') return c-'0';
    if (c>='a'&&c<='f') return c-'a'+10;
    if (c>='A'&&c<='F') return c-'A'+10;
    return -1;
}

/* Parse 64 hex chars -> 32 bytes. Returns 1 on success. */
static int parse_hex32(const char *s, u8 out[32]) {
    if (strlen(s) < 64) return 0;
    for (int i = 0; i < 32; i++) {
        int hi = hex_nibble(s[i*2]), lo = hex_nibble(s[i*2+1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (u8)((hi<<4)|lo);
    }
    return 1;
}

/* Resolve text to token[32]: 64-hex, else sha256(text), else lookup name in instruction_names. */
static void resolve_token_text(const char *s, u8 out[32]) {
    memset(out, 0, 32);
    if (parse_hex32(s, out)) return;
    load_names();
    for (u32 i = 0; i < g_n; i++) {
        if (!_stricmp(g_ent[i].name, s)) { memcpy(out, g_ent[i].token, 32); return; }
    }
    cvm_sha256((const u8*)s, (u32)strlen(s), out);
}

/*
 * payload: typein var id
 * stack: u32 byte offset of instruction
 *
 * Edits:
 *  var_read_payload: text -> id bytes
 *  var_write/set_payload: id_len+id (+size rules like var_edit_apply simplified)
 *  cond_token_payload: text -> target token (keeps uid/flags)
 *  token_run_by_hand: text -> target token (keeps uid)
 *  jump/exec/cond_payload: text -> 32-byte target
 */
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
    while (z < tsize && text[z] && z < sizeof(buf)-1) { buf[z] = (char)text[z]; z++; }
    buf[z] = 0; trim(buf);
    if (!buf[0]) { cont(); return; }

    u8 *b = cvm_cached_base();
    u32 l = cvm_cached_len();
    if (!bl_ok(b, l, off) || bl_is_end(b + off)) { cont(); return; }
    if (bl_tlen(b + off) != 32) { cont(); return; }
    u8 tok[32]; memcpy(tok, bl_token_c(b + off), 32);
    u32 pn = bl_plen(b + off);
    const u8 *oldp = bl_payload_c(b + off);
    const char *nm = tok_name(tok);

    u8 npay[512];
    u32 nn = 0;

    if (nm && (!strcmp(nm, "var_read_payload"))) {
        nn = (u32)strlen(buf); if (nn > 256) nn = 256;
        memcpy(npay, buf, nn);
        replace_payload(off, npay, nn);
        cont(); return;
    }
    if (nm && (!strcmp(nm, "var_write_payload") || !strcmp(nm, "var_set_payload"))) {
        u32 idn = (u32)strlen(buf); if (idn > 256) idn = 256;
        *(u32*)npay = idn;
        memcpy(npay+4, buf, idn);
        /* preserve trailing size/data if set and rest==4 */
        if (!strcmp(nm, "var_set_payload") && pn >= 4) {
            u32 oid = *(u32*)oldp;
            if (oid <= 256 && pn == 4 + oid + 4) {
                memcpy(npay+4+idn, oldp+4+oid, 4);
                nn = 4 + idn + 4;
            } else nn = 4 + idn;
        } else nn = 4 + idn;
        replace_payload(off, npay, nn);
        cont(); return;
    }
    if (nm && !strcmp(nm, "cond_token_payload")) {
        u8 target[32]; resolve_token_text(buf, target);
        u32 uid = (pn >= 36) ? *(u32*)(oldp+32) : 1;
        u8 once = (pn >= 38) ? oldp[36] : 0;
        u8 contf = (pn >= 38) ? oldp[37] : 1;
        memcpy(npay, target, 32);
        *(u32*)(npay+32) = uid ? uid : 1;
        npay[36] = once; npay[37] = contf; npay[38]=0; npay[39]=0;
        replace_payload(off, npay, 40);
        cont(); return;
    }
    if (nm && !strcmp(nm, "token_run_by_hand")) {
        u8 target[32]; resolve_token_text(buf, target);
        u32 uid = (pn >= 4) ? *(u32*)oldp : 1;
        if (!uid) uid = 1;
        *(u32*)npay = uid;
        memcpy(npay+4, target, 32);
        replace_payload(off, npay, 36);
        cont(); return;
    }
    if (nm && (!strcmp(nm, "cond_payload") || !strcmp(nm, "jump_payload") || !strcmp(nm, "exec_payload"))) {
        u8 target[32]; resolve_token_text(buf, target);
        replace_payload(off, target, 32);
        cont(); return;
    }
    cont();
}
