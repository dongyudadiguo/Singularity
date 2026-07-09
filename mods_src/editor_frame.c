#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_current_key(void);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
extern __declspec(dllimport) void cvm_cached_set_len(u32 n);
extern __declspec(dllimport) void cvm_cache_flush(void);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);
extern __declspec(dllimport) u32 cvm_children(const H parent, H *out, u32 cap);
extern __declspec(dllimport) u32 cvm_file_read(const H h, u8 *out, u32 cap);
extern __declspec(dllimport) void cvm_edge(const H parent, const H child);

#include "../dxgfx.h"

#define MAX_REG 2048
#define MAX_VIEWS 64
#define MAX_COPY (1u<<18)
#define MAX_BLOCK (1u<<20)

typedef struct { H token; char name[96]; int tag; } RegEntry;
typedef struct { H key; float x, y; int used; int linked; int link_view; float link_x, link_y; } View;

#define MAX_NC 4096
typedef struct { H tok; char nm[96]; } NCEntry;

typedef struct {
    int ready;
    H registry_key;
    H root_key;
    RegEntry reg[MAX_REG];
    u32 reg_count;
    View views[MAX_VIEWS];
    u32 view_count;
    u32 active_view;
    u32 point_off;
    u32 mark_off;
    int marking;
    float cam_x, cam_y, zoom;
    float last_mx, last_my;
    char input[256];
    char completion[96];
    u32 completion_index;
    u8 copy[MAX_COPY];
    u32 copy_len;
    int dirty;
    /* Context menu */
    int menu_active;
    int menu_view;
    u32 menu_off;
    H menu_token;
    u32 menu_psize;
    float menu_mx, menu_my;
    int dragging_view;
    /* Name cache */
    NCEntry nc[MAX_NC];
    u32 nc_count;
} Editor;

static Editor E;

static int zero32(const u8 *p){ for(int i=0;i<32;i++) if(p[i]) return 0; return 1; }
static u32 ins_size(const u8 *p){ return 36u + *(u32*)(p + 32); }
static int valid_off(u8 *b, u32 len, u32 off){ return len >= 32 && off <= len - 32 && !zero32(b + off) && off + 36 <= len && off + ins_size(b + off) <= len; }
static int key_same(const H a, const H b){ return memcmp(a,b,32)==0; }
static int printable_ascii(const u8 *p, u32 n){ for(u32 i=0;i<n;i++) if(p[i]<32 || p[i]>126) return 0; return 1; }
static int looks_like_dll(const u8 *p, u32 n){ return n >= 2 && p[0] == 'M' && p[1] == 'Z'; }

static void hex8(const H h, char *out){ static const char x[]="0123456789abcdef"; for(int i=0;i<4;i++){ out[i*2]=x[h[i]>>4]; out[i*2+1]=x[h[i]&15]; } out[8]=0; }

static void hex64(const H h, char *out){ static const char x[]="0123456789abcdef"; for(int i=0;i<32;i++){ out[i*2]=x[h[i]>>4]; out[i*2+1]=x[h[i]&15]; } out[64]=0; }

/* PE export reader */
#define MAX_EXPORTS 64

static int pe_read_exports(const char *path, char exports[MAX_EXPORTS][64], int *count) {
    FILE *f; long fsize; u8 *data; u32 pe_off; u16 num_sec, opt_sz, magic;
    u32 export_rva, exp_off, num_names, addr_names_rva;
    u32 sec_start, i, j;
    typedef struct { u32 vrva, vsize, rptr; } Sec;
    Sec secs[96]; int nsec;

    *count = 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); fsize = ftell(f); fseek(f, 0, SEEK_SET);
    if (fsize < 64) { fclose(f); return 0; }
    data = (u8*)malloc((u32)fsize);
    if (!data) { fclose(f); return 0; }
    { size_t rd = fread(data, 1, (u32)fsize, f); fclose(f);
      if (rd != (size_t)fsize) { free(data); return 0; }
    }

    if (data[0]!='M'||data[1]!='Z') { free(data); return 0; }
    pe_off = *(u32*)(data+0x3C);
    if (pe_off+24>(u32)fsize||data[pe_off]!='P'||data[pe_off+1]!='E') { free(data); return 0; }
    num_sec = *(u16*)(data+pe_off+6);
    opt_sz = *(u16*)(data+pe_off+20);
    magic = *(u16*)(data+pe_off+24);
    if (magic==0x20b) export_rva = *(u32*)(data+pe_off+24+112);
    else export_rva = *(u32*)(data+pe_off+24+96);
    if (!export_rva) { free(data); return 0; }

    sec_start = pe_off + 24 + opt_sz;
    nsec = num_sec < 96 ? num_sec : 96;
    for (i=0; i<(u32)nsec; i++) {
        u32 off = sec_start + i*40;
        if (off+40 > (u32)fsize) { free(data); return 0; }
        secs[i].vsize = *(u32*)(data+off+8);
        secs[i].vrva = *(u32*)(data+off+12);
        secs[i].rptr = *(u32*)(data+off+20);
    }

    exp_off = 0;
    for (i=0; i<(u32)nsec; i++)
        if (secs[i].vrva<=export_rva && export_rva<secs[i].vrva+secs[i].vsize) {
            exp_off = secs[i].rptr + (export_rva - secs[i].vrva); break; }
    if (!exp_off||exp_off+40>(u32)fsize) { free(data); return 0; }

    num_names = *(u32*)(data+exp_off+24);
    addr_names_rva = *(u32*)(data+exp_off+32);

    for (i=0; i<num_names && *count<MAX_EXPORTS; i++) {
        u32 nrva, nfile, noff = addr_names_rva + i*4;
        for (j=0; j<(u32)nsec; j++)
            if (secs[j].vrva<=noff && noff<secs[j].vrva+secs[j].vsize) {
                noff = secs[j].rptr + (noff - secs[j].vrva); goto got_nptr; }
        continue;
        got_nptr:
        if (noff+4>(u32)fsize) continue;
        nrva = *(u32*)(data+noff);
        nfile = 0;
        for (j=0; j<(u32)nsec; j++)
            if (secs[j].vrva<=nrva && nrva<secs[j].vrva+secs[j].vsize) {
                nfile = secs[j].rptr + (nrva - secs[j].vrva); break; }
        if (!nfile||nfile>=(u32)fsize) continue;
        { int len=0;
          while (nfile+len<(u32)fsize && data[nfile+len] && len<63)
            { exports[*count][len]=data[nfile+len]; len++; }
          exports[*count][len]=0;
        }
        (*count)++;
    }
    free(data);
    return 1;
}

static const char *name_for(const H token) {
    for (u32 i=0;i<E.reg_count;i++) if (!E.reg[i].tag && key_same(E.reg[i].token, token)) return E.reg[i].name;
    for (u32 i=0;i<E.nc_count;i++) if (key_same(E.nc[i].tok, token)) return E.nc[i].nm;
    static char tmp[16]; hex8(token, tmp); return tmp;
}
static void nc_add(const H tok, const char *nm) {
    if (!nm||!nm[0]||E.nc_count>=MAX_NC) return;
    for (u32 i=0;i<E.nc_count;i++) if (key_same(E.nc[i].tok,tok)) return;
    memcpy(E.nc[E.nc_count].tok,tok,32);
    strncpy(E.nc[E.nc_count].nm,nm,95); E.nc[E.nc_count].nm[95]=0;
    E.nc_count++;
}
static void nc_save(void) {
    FILE *f=fopen("name_cache.bin","wb"); if(!f) return;
    fwrite(&E.nc_count,4,1,f);
    fwrite(E.nc,sizeof(NCEntry),E.nc_count,f);
    fclose(f);
}
static void nc_load(void) {
    FILE *f=fopen("name_cache.bin","rb"); if(!f) return;
    if(fread(&E.nc_count,4,1,f)!=1){fclose(f);return;}
    if(E.nc_count>MAX_NC) E.nc_count=MAX_NC;
    fread(E.nc,sizeof(NCEntry),E.nc_count,f);
    fclose(f);
}

/* Scan mods/ directory, build signature->name map from known nc entries,
   then resolve all unknown DLLs by matching their export signature + file size. */
/* Scan mods/ directory, build export-signature->name map from known nc entries,
   then resolve all unknown DLLs by matching their export function signatures.
   For unique signatures (e.g. editor_state_init,run -> editor_frame), this works
   across different DLL versions. For ambiguous signatures (e.g. "run" used by 400+
   DLLs), we fall back to hex hash. */
/* Scan mods/ directory, build export-signature->name map from known nc entries,
   then resolve all unknown DLLs. Uses two-tier matching:
   1) Signature-only match (for unique sigs like editor_state_init,run -> editor_frame)
   2) Signature+size match (for "run" sigs where size disambiguates: run+38298 -> editor_init) */
static void nc_scan_mods(void) {
    WIN32_FIND_DATAA fd;
    HANDLE fh;
    char hex[65], path[260];
    char exports[MAX_EXPORTS][64];
    int nexp;

    /* Phase 1: Build sig -> (name, size) from nc entries */
    #define MAX_SM 256
    typedef struct { char sig[512]; u32 size; char name[96]; } SigEntry;
    static SigEntry sm[MAX_SM];
    int sm_count = 0;

    for (u32 i = 0; i < E.nc_count && sm_count < MAX_SM; i++) {
        hex64(E.nc[i].tok, hex);
        snprintf(path, sizeof(path), "mods/%s.dll", hex);
        if (!pe_read_exports(path, exports, &nexp)) continue;
        if (nexp == 0) continue;

        for (int a = 0; a < nexp-1; a++)
            for (int b = a+1; b < nexp; b++)
                if (strcmp(exports[a], exports[b]) > 0) {
                    char tmp[64]; strcpy(tmp, exports[a]);
                    strcpy(exports[a], exports[b]); strcpy(exports[b], tmp);
                }

        char sig[512] = "";
        for (int a = 0; a < nexp; a++) {
            if (a) strcat(sig, ",");
            strcat(sig, exports[a]);
        }

        FILE *f = fopen(path, "rb");
        u32 fsize = 0;
        if (f) { fseek(f, 0, SEEK_END); fsize = (u32)ftell(f); fclose(f); }

        int found = -1;
        for (int j = 0; j < sm_count; j++) {
            if (sm[j].size == fsize && strcmp(sm[j].sig, sig) == 0) { found = j; break; }
        }
        if (found >= 0) {
            if (strcmp(sm[found].name, E.nc[i].nm) != 0) {
                sm[found].name[0] = '?'; sm[found].name[1] = 0;
            }
        } else {
            strncpy(sm[sm_count].sig, sig, 511); sm[sm_count].sig[511] = 0;
            sm[sm_count].size = fsize;
            strncpy(sm[sm_count].name, E.nc[i].nm, 95); sm[sm_count].name[95] = 0;
            sm_count++;
        }
    }

    /* Phase 2: Scan all DLLs */
    fh = FindFirstFileA("mods/*.dll", &fd);
    if (fh == INVALID_HANDLE_VALUE) return;

    int added = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        strncpy(hex, fd.cFileName, 64); hex[64] = 0;
        char *dot = strrchr(hex, '.');
        if (dot) *dot = 0;
        if (strlen(hex) != 64) continue;

        H tok;
        for (int j = 0; j < 32; j++) {
            int hi = hex[j*2]>='a'?hex[j*2]-'a'+10:hex[j*2]>='A'?hex[j*2]-'A'+10:hex[j*2]-'0';
            int lo = hex[j*2+1]>='a'?hex[j*2+1]-'a'+10:hex[j*2+1]>='A'?hex[j*2+1]-'A'+10:hex[j*2+1]-'0';
            tok[j] = (u8)(hi*16+lo);
        }
        int already = 0;
        for (u32 i = 0; i < E.nc_count; i++)
            if (key_same(E.nc[i].tok, tok)) { already = 1; break; }
        if (already) continue;

        snprintf(path, sizeof(path), "mods/%s", fd.cFileName);
        if (!pe_read_exports(path, exports, &nexp)) continue;
        if (nexp == 0) continue;

        for (int a = 0; a < nexp-1; a++)
            for (int b = a+1; b < nexp; b++)
                if (strcmp(exports[a], exports[b]) > 0) {
                    char tmp2[64]; strcpy(tmp2, exports[a]);
                    strcpy(exports[a], exports[b]); strcpy(exports[b], tmp2);
                }

        char sig[512] = "";
        for (int a = 0; a < nexp; a++) {
            if (a) strcat(sig, ",");
            strcat(sig, exports[a]);
        }

        FILE *f = fopen(path, "rb");
        u32 fsize = 0;
        if (f) { fseek(f, 0, SEEK_END); fsize = (u32)ftell(f); fclose(f); }

        char *best = NULL;
        for (int j = 0; j < sm_count; j++) {
            if (sm[j].name[0] == '?') continue;
            if (strcmp(sm[j].sig, sig) != 0) continue;
            if (!best) { best = sm[j].name; continue; }
            if (sm[j].size == fsize) { best = sm[j].name; break; }
        }

        if (best) {
            nc_add(tok, best);
            added++;
        }
    } while (FindNextFileA(fh, &fd));
    FindClose(fh);
}

static void append_reg(const H token, const char *name, int tag) {
    if (E.reg_count >= MAX_REG) return;
    for (u32 i=0;i<E.reg_count;i++) if (key_same(E.reg[i].token, token)) return;
    memcpy(E.reg[E.reg_count].token, token, 32);
    strncpy(E.reg[E.reg_count].name, name && *name ? name : "?", sizeof(E.reg[E.reg_count].name)-1);
    E.reg[E.reg_count].tag = tag;
    E.reg_count++;
}

static int registry_child_name(const H token, char *name, u32 cap) {
    H kids[64];
    u32 n = cvm_children(token, kids, 64);
    if (n > 64) n = 64;
    for (u32 i=0;i<n;i++) {
        memset(name,0,cap);
        u32 got = cvm_file_read(kids[i], (u8*)name, cap-1);
        u32 chk = got < cap-1 ? got : cap-1;
        if (got && name[0] != '#' && printable_ascii((u8*)name, chk) && !looks_like_dll((u8*)name, got)) return 1;
    }
    name[0] = 0;
    return 0;
}

static void load_registry_rec(const H parent, int depth) {
    if (depth > 6 || E.reg_count >= MAX_REG) return;
    H kids[256];
    u32 n = cvm_children(parent, kids, 256);
    if (n > 256) n = 256;
    for (u32 i=0;i<n;i++) {
        char name[96];
        memset(name,0,sizeof(name));
        u32 got = cvm_file_read(kids[i], (u8*)name, sizeof(name)-1);
        int is_tag = got > 0 && name[0] == '#';
        if (is_tag) {
            append_reg(kids[i], name, 1);
            load_registry_rec(kids[i], depth+1);
        } else if (registry_child_name(kids[i], name, sizeof(name))) {
            append_reg(kids[i], name, 0);
        }
    }
}

__declspec(dllexport) int editor_state_init(const H current_key, const H registry_key) {
    if (E.ready) return 1;
    memset(&E, 0, sizeof(E));
    memcpy(E.root_key, current_key, 32);
    memcpy(E.registry_key, registry_key, 32);
    E.zoom = 1.0f;
    E.dragging_view = -1;
    E.views[0].used = 1;
    memcpy(E.views[0].key, current_key, 32);
    E.views[0].x = 0;
    E.views[0].y = 0;
    E.view_count = 1;
    load_registry_rec(registry_key, 0);
    nc_load();
    for(u32 i=0;i<E.reg_count;i++)
        if(!E.reg[i].tag && E.reg[i].name[0] && E.reg[i].name[0]!='?')
            nc_add(E.reg[i].token, E.reg[i].name);
    nc_scan_mods();
    nc_save();
    E.ready = 1;
    return 1;
}

static int match_input(const char *name, const char *in) {
    if (!in[0]) return 0;
    while (*name && *in) {
        char a=*name++, b=*in++;
        if (a=='_') { in--; continue; }
        if (b=='_') { name--; continue; }
        if (a>='A'&&a<='Z') a += 32;
        if (b>='A'&&b<='Z') b += 32;
        if (a != b) return 0;
    }
    return *in == 0;
}

static void update_completion(void) {
    E.completion[0]=0; E.completion_index=0xffffffffu;
    if (!E.input[0]) return;
    for (u32 i=0;i<E.reg_count;i++) {
        if (E.reg[i].tag) continue;
        if (match_input(E.reg[i].name, E.input)) {
            strncpy(E.completion, E.reg[i].name, sizeof(E.completion)-1);
            E.completion_index = i;
            return;
        }
    }
}

static int command_key_pressed(u8 *pressed) {
    return pressed[VK_SPACE] || pressed[VK_TAB] || pressed[VK_RMENU] || pressed[VK_LMENU] ||
           pressed[VK_OEM_3] || pressed[VK_DELETE] || pressed[VK_INSERT] ||
           pressed[VK_BACK] || pressed[VK_ESCAPE] || pressed[VK_RETURN];
}

static void clear_input(void) {
    E.input[0] = 0;
    E.completion[0] = 0;
    E.completion_index = 0xffffffffu;
}

static void load_view(u32 vi) {
    H h;
    cvm_resolve_payload_hash(E.views[vi].key, h);
}

static void insert_raw(u32 off, const H token, const u8 *payload, u32 pn) {
    u8 *b=cvm_cached_base(); u32 len=cvm_cached_len();
    if (len < 32 || off > len-32 || len + 36 + pn > MAX_BLOCK) return;
    memmove(b+off+36+pn, b+off, len-off);
    memcpy(b+off, token, 32);
    *(u32*)(b+off+32)=pn;
    if (pn) memcpy(b+off+36, payload, pn);
    cvm_cached_set_len(len+36+pn);
    E.point_off = off + 36 + pn;
    E.dirty = 1;
}

static void delete_at(u32 off) {
    u8 *b=cvm_cached_base(); u32 len=cvm_cached_len();
    if (!valid_off(b,len,off)) return;
    u32 n=ins_size(b+off);
    memmove(b+off, b+off+n, len-off-n);
    cvm_cached_set_len(len-n);
    E.point_off = off;
    E.dirty = 1;
}

static void insert_data_text(void) {
    if (!E.input[0]) return;
    H tok; const char label[]="data"; cvm_sha256((const u8*)label, 4, tok);
    insert_raw(E.point_off, tok, (const u8*)E.input, (u32)strlen(E.input));
    clear_input();
}

static void insert_completion(void) {
    if (E.completion_index == 0xffffffffu) return;
    RegEntry *r=&E.reg[E.completion_index];
    insert_raw(E.point_off, r->token, 0, 0);
    clear_input();
}

static u32 find_or_add_view(const H key, float x, float y, int linked, int link_view, float link_x, float link_y) {
    for (u32 i=0;i<E.view_count;i++) {
        if (E.views[i].used && key_same(E.views[i].key, key)) {
            E.views[i].linked = linked;
            E.views[i].link_view = link_view;
            E.views[i].link_x = link_x;
            E.views[i].link_y = link_y;
            return i;
        }
    }
    if (E.view_count >= MAX_VIEWS) return 0xffffffffu;
    View *v=&E.views[E.view_count];
    memset(v,0,sizeof(*v));
    v->used=1;
    memcpy(v->key,key,32);
    v->x=x;
    v->y=y;
    v->linked=linked;
    v->link_view=link_view;
    v->link_x=link_x;
    v->link_y=link_y;
    return E.view_count++;
}

static void create_child_block(void) {
    char nm[96]; H k; u8 z[32]={0};
    if (E.input[0]) snprintf(nm,sizeof(nm),"%s",E.input); else snprintf(nm,sizeof(nm),"block%u",E.view_count);
    cvm_sha256((const u8*)nm, (u32)strlen(nm), k);
    cvm_edge(k, z);
    insert_raw(E.point_off, k, 0, 0);
    if (E.view_count < MAX_VIEWS) {
        View *v=&E.views[E.view_count++]; v->used=1; memcpy(v->key,k,32); v->x=E.views[E.active_view].x+360; v->y=E.views[E.active_view].y;
    }
    clear_input();
}

static void copy_range(u32 a, u32 b) {
    if (a>b) { u32 t=a; a=b; b=t; }
    u8 *base=cvm_cached_base(); u32 len=cvm_cached_len();
    if (a>len || b>len || b-a>MAX_COPY) return;
    memcpy(E.copy, base+a, b-a); E.copy_len=b-a;
}

static void paste_copy(void) {
    u8 *b=cvm_cached_base(); u32 len=cvm_cached_len();
    if (!E.copy_len || len+E.copy_len>MAX_BLOCK || E.point_off>len-32) return;
    memmove(b+E.point_off+E.copy_len,b+E.point_off,len-E.point_off);
    memcpy(b+E.point_off,E.copy,E.copy_len);
    cvm_cached_set_len(len+E.copy_len); E.point_off += E.copy_len; E.dirty=1;
}

static void draw_payload_summary(u8 *p, u32 n, float x, float y) {
    if (!n) return;
    char buf[120]; u32 m=n<48?n:48;
    int printable=1; for(u32 i=0;i<m;i++) if(p[i]<32 || p[i]>126) printable=0;
    if (printable) { snprintf(buf,sizeof(buf),"  \'%.*s\'",(int)m,(char*)p); }
    else { snprintf(buf,sizeof(buf),"  [%u bytes]",n); }
    dxgfx_draw_text((int)x,(int)y,0xff7cc6ff,18.0f,buf,(u32)strlen(buf));
}

static void draw_view(u32 vi, float mwx, float mwy, int mouse_pressed, int cx, int cy) {
    load_view(vi);
    u8 *b=cvm_cached_base(); u32 len=cvm_cached_len();
    float x=E.views[vi].x, y=E.views[vi].y;
    char title[160], hx[16]; hex8(E.views[vi].key,hx);
    if (E.views[vi].linked) {
        float lx = E.views[vi].link_x, ly = E.views[vi].link_y;
        if (E.views[vi].link_view >= 0 && (u32)E.views[vi].link_view < E.view_count) {
            lx = E.views[E.views[vi].link_view].x + E.views[vi].link_x;
            ly = E.views[E.views[vi].link_view].y + E.views[vi].link_y;
        }
        dxgfx_draw_line(lx, ly, x, y, 0xff7bd88f, 2.0f);
    }
    snprintf(title,sizeof(title),"[%u] %s",vi,hx);
    dxgfx_draw_text((int)x,(int)(y-26),0xffcfcfcf,18.0f,title,(u32)strlen(title));
    if (mwx>=x && mwx<=x+520 && mwy>=y-28 && mwy<=y-4 && (mouse_pressed & 2)) {
        E.active_view = vi;
        E.dragging_view = (int)vi;
    }
    u32 off=0; float row_y=y;
    while (off+32<=len) {
        if (zero32(b+off)) break;
        if (!valid_off(b,len,off)) break;
        const char *nm=name_for(b+off);
        int selected=(vi==E.active_view && off==E.point_off);
        float tw = (float)(strlen(nm) * 10 + 20);
        if (mwx>=x && mwx<=x+tw && mwy>=row_y && mwy<=row_y+22) {
            if (mouse_pressed & 1) { E.active_view=vi; E.point_off=off; }
            if (mouse_pressed & 2) {
                /* Only the list header drags a list.  Right-dragging any token row
                   enters/expands that token and drags the newly opened list. */
                u32 ni = find_or_add_view(b+off, mwx, mwy, 1, (int)vi, 72.0f, row_y - y + 10.0f);
                if (ni != 0xffffffffu) {
                    E.active_view = ni;
                    E.dragging_view = (int)ni;
                }
            }
        }
        if (selected) dxgfx_draw_rect(x-8,row_y-2,tw+8,22,0xff3f4d5a,1,1);
        dxgfx_draw_text((int)x,(int)row_y, selected?0xffffffff:0xffe8e8e8,18.0f,nm,(u32)strlen(nm));
        draw_payload_summary(b+off+36, *(u32*)(b+off+32), x+180, row_y);
        row_y += 22;
        off += ins_size(b+off);
    }
    float end_tw = 60.0f; { if (mwx>=x && mwx<=x+end_tw && mwy>=row_y && mwy<=row_y+22 && (mouse_pressed&1)) { E.active_view=vi; E.point_off=off; } }
    if (vi==E.active_view && E.point_off==off) dxgfx_draw_rect(x-8,row_y-2,end_tw+8,22,0xff3f4d5a,1,1);
    dxgfx_draw_text((int)x,(int)row_y,0xff777777,18.0f,"<end>",5);
    (void)cx; (void)cy;
}

static void handle_input(u8 *down, u8 *pressed, u8 *released, int *mouse, char *text, float mwx, float mwy) {
    if (pressed[VK_ESCAPE]) clear_input();
    size_t l=strlen(E.input);
    if (text[0] && !command_key_pressed(pressed) && l < sizeof(E.input)-1) strncat(E.input,text,sizeof(E.input)-1-l);
    if (pressed[VK_BACK] && l) E.input[l-1]=0;
    update_completion();

    load_view(E.active_view);
    if (pressed[VK_SPACE]) insert_completion();
    if (pressed[VK_TAB]) { insert_completion(); }
    if (pressed[VK_RMENU] || pressed[VK_LMENU]) create_child_block();
    if (pressed[VK_OEM_3]) insert_data_text();
    if (pressed[VK_DELETE]) { E.mark_off=E.point_off; E.marking=1; }
    if (released[VK_DELETE] && E.marking) { delete_at(E.mark_off); E.marking=0; }
    if (pressed[VK_LSHIFT] || pressed[VK_RSHIFT]) { E.mark_off=E.point_off; }
    if ((released[VK_LSHIFT] || released[VK_RSHIFT])) copy_range(E.mark_off,E.point_off);
    if (pressed[VK_INSERT]) paste_copy();
    if (down[VK_CONTROL] && pressed['S']) { cvm_cache_flush(); E.dirty=0; }
    if (mouse[5]) { E.zoom += (float)mouse[5] * (0.1f * E.zoom); if(E.zoom<0.1f)E.zoom=0.1f; if(E.zoom>8.0f)E.zoom=8.0f; }
    if (mouse[2] & 4) { E.cam_x -= ((float)mouse[0]-E.last_mx)/E.zoom; E.cam_y -= ((float)mouse[1]-E.last_my)/E.zoom; }
    if (!(mouse[2] & 2)) E.dragging_view = -1;
    if ((mouse[2] & 2) && E.dragging_view >= 0 && (u32)E.dragging_view < E.view_count) {
        E.views[E.dragging_view].x += ((float)mouse[0]-E.last_mx)/E.zoom;
        E.views[E.dragging_view].y += ((float)mouse[1]-E.last_my)/E.zoom;
    }
    E.last_mx=(float)mouse[0]; E.last_my=(float)mouse[1];
}

static void draw_hud(int mx, int my) {
    char hud[420];
    const char *match = E.completion[0] ? E.completion : "";
    if (E.reg_count == 0) snprintf(hud,sizeof(hud),"%s%s registry:empty%s",E.input,match,E.dirty?" *":"");
    else snprintf(hud,sizeof(hud),"%s%s%s",E.input,match,E.dirty?" *":"");
    dxgfx_draw_rect((float)mx+14,(float)my-2,360,24,0xcc101214,1,1);
    dxgfx_draw_text(mx+20,my,0xffffffff,18.0f,hud,(u32)strlen(hud));
}

__declspec(dllexport) void run(void) {
    static int frame_num = 0;
    clock_t frame_start = clock();
    if (!E.ready) {
        H tag; const char s[]="#TAG";
        cvm_sha256((const u8*)s,4,tag);
        editor_state_init(cvm_current_key(), tag);
    }
    u8 down[256], pressed[256], released[256]; int mouse[8]; char text[64]; float wm[2]={0,0};
    dxgfx_input_snapshot(down,pressed,released,mouse,text);
    dxgfx_set_camera(E.cam_x,E.cam_y,E.zoom);
    dxgfx_world_mouse(wm);
    if (dxgfx_window_should_close()) ExitProcess(0);
    handle_input(down,pressed,released,mouse,text,wm[0],wm[1]);

    dxgfx_frame_begin();
    dxgfx_clear(0xff101214);
    dxgfx_set_camera(E.cam_x,E.cam_y,E.zoom);
    for (u32 i=0;i<E.view_count;i++) draw_view(i,wm[0],wm[1],mouse[3],mouse[0],mouse[1]);
    dxgfx_set_camera(640.0f,360.0f,1.0f);
    draw_hud(mouse[0], mouse[1]);
    /* Context menu */
    if (E.menu_active) {
        int dismissed = 0;
        if (pressed[VK_ESCAPE]) { E.menu_active=0; dismissed=1; }
        if (!dismissed && (mouse[3] & 1)) {
            float mx=(float)mouse[0], my=(float)mouse[1];
            if (mx>=E.menu_mx && mx<=E.menu_mx+220 && my>=E.menu_my && my<=E.menu_my+24) {
                if (E.view_count < MAX_VIEWS) {
                    View *v=&E.views[E.view_count++]; v->used=1;
                    memcpy(v->key, E.menu_token, 32);
                    v->x=E.views[E.menu_view].x+360; v->y=E.views[E.menu_view].y;
                }
            }
            E.menu_active=0; dismissed=1;
        }
        if (!dismissed) {
            E.menu_mx=(float)mouse[0]+10; E.menu_my=(float)mouse[1]-10;
            dxgfx_draw_rect(E.menu_mx-4,E.menu_my-4,228,32,0xcc1a1d21,1,1);
            dxgfx_draw_rect(E.menu_mx-4,E.menu_my-4,228,32,0xff4a6080,1,0);
            dxgfx_draw_text((int)E.menu_mx,(int)E.menu_my,0xffffffff,18.0f,"Open block in new view",22);
        }
    }
    dxgfx_frame_end();
    clock_t frame_end = clock();
    printf("[frame] #%d time=%lu ms views=%u active=%u\n",
           frame_num++,
           (unsigned long)((frame_end-frame_start)*1000/CLOCKS_PER_SEC),
           E.view_count, E.active_view);
    Sleep(8);
    cont();
}
