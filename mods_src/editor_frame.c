#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned char u8;
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
typedef struct { H key; float x, y; int used; } View;

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
} Editor;

static Editor E;

static int zero32(const u8 *p){ for(int i=0;i<32;i++) if(p[i]) return 0; return 1; }
static u32 ins_size(const u8 *p){ return 36u + *(u32*)(p + 32); }
static int valid_off(u8 *b, u32 len, u32 off){ return len >= 32 && off <= len - 32 && !zero32(b + off) && off + 36 <= len && off + ins_size(b + off) <= len; }
static int key_same(const H a, const H b){ return memcmp(a,b,32)==0; }
static int printable_ascii(const u8 *p, u32 n){ for(u32 i=0;i<n;i++) if(p[i]<32 || p[i]>126) return 0; return 1; }
static int looks_like_dll(const u8 *p, u32 n){ return n >= 2 && p[0] == 'M' && p[1] == 'Z'; }

static void hex8(const H h, char *out){ static const char x[]="0123456789abcdef"; for(int i=0;i<4;i++){ out[i*2]=x[h[i]>>4]; out[i*2+1]=x[h[i]&15]; } out[8]=0; }

static const char *name_for(const H token) {
    for (u32 i=0;i<E.reg_count;i++) if (!E.reg[i].tag && key_same(E.reg[i].token, token)) return E.reg[i].name;
    static char tmp[16]; hex8(token, tmp); return tmp;
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
    E.views[0].used = 1;
    memcpy(E.views[0].key, current_key, 32);
    E.views[0].x = 0;
    E.views[0].y = 0;
    E.view_count = 1;
    load_registry_rec(registry_key, 0);
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
    if (printable) { snprintf(buf,sizeof(buf),"  '%.*s'",(int)m,(char*)p); }
    else { snprintf(buf,sizeof(buf),"  [%u bytes]",n); }
    dxgfx_draw_text((int)x,(int)y,0xff7cc6ff,18.0f,buf,(u32)strlen(buf));
}

static void draw_view(u32 vi, float mwx, float mwy, int mouse_pressed) {
    load_view(vi);
    u8 *b=cvm_cached_base(); u32 len=cvm_cached_len();
    float x=E.views[vi].x, y=E.views[vi].y;
    char title[160], hx[16]; hex8(E.views[vi].key,hx);
    snprintf(title,sizeof(title),"[%u] %s",vi,hx);
    dxgfx_draw_text((int)x,(int)(y-26),0xffcfcfcf,18.0f,title,(u32)strlen(title));
    u32 off=0; float cy=y;
    while (off+32<=len) {
        if (zero32(b+off)) break;
        if (!valid_off(b,len,off)) break;
        const char *nm=name_for(b+off);
        int selected=(vi==E.active_view && off==E.point_off);
        if (mwx>=x && mwx<=x+520 && mwy>=cy && mwy<=cy+22) {
            if (mouse_pressed & 1) { E.active_view=vi; E.point_off=off; }
        }
        if (selected) dxgfx_draw_rect(x-8,cy-2,520,22,0xff3f4d5a,1,1);
        dxgfx_draw_text((int)x,(int)cy, selected?0xffffffff:0xffe8e8e8,18.0f,nm,(u32)strlen(nm));
        draw_payload_summary(b+off+36, *(u32*)(b+off+32), x+180, cy);
        cy += 22;
        off += ins_size(b+off);
    }
    if (mwx>=x && mwx<=x+520 && mwy>=cy && mwy<=cy+22 && (mouse_pressed&1)) { E.active_view=vi; E.point_off=off; }
    if (vi==E.active_view && E.point_off==off) dxgfx_draw_rect(x-8,cy-2,520,22,0xff3f4d5a,1,1);
    dxgfx_draw_text((int)x,(int)cy,0xff777777,18.0f,"<end>",5);
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
    if (mouse[2] & 2) { E.views[E.active_view].x += ((float)mouse[0]-E.last_mx)/E.zoom; E.views[E.active_view].y += ((float)mouse[1]-E.last_my)/E.zoom; }
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
    for (u32 i=0;i<E.view_count;i++) draw_view(i,wm[0],wm[1],mouse[3]);
    dxgfx_set_camera(640.0f,360.0f,1.0f);
    draw_hud(mouse[0], mouse[1]);
    dxgfx_frame_end();
    clock_t frame_end = clock();
    printf("[frame] #%d time=%lu ms views=%u active=%u\n",
           frame_num++,
           (unsigned long)((frame_end-frame_start)*1000/CLOCKS_PER_SEC),
           E.view_count, E.active_view);
    Sleep(8);
    cont();
}
