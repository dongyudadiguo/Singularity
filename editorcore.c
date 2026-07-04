#define WIN32_LEAN_AND_MEAN
#define EDITORCORE_BUILD
#include "editorcore.h"
#include "dxgfx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"advapi32.lib")

typedef ec_u8 H[32];
typedef struct { char name[96]; H token; } Reg;
typedef struct { float x,y,w,h; H key; unsigned offset; } View;

static int inited;
static int should_halt;
static Reg regs[512];
static int regn;
static View views[8];
static int viewn;
static int selected_view;
static unsigned selected_offset;
static char input[128];
static int input_len;
static H root_key;
static H user_id;
static float cam_x, cam_y, zoom = 1.0f;
static int last_mouse[4];

static int sha256(const ec_u8 *p, ec_u32 n, H out) {
    HCRYPTPROV prov=0; HCRYPTHASH hash=0; DWORD len=32; int ok=0;
    if(!CryptAcquireContextA(&prov,0,0,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)) goto done;
    if(!CryptCreateHash(prov,CALG_SHA_256,0,0,&hash)) goto done;
    if(!CryptHashData(hash,p,n,0)) goto done;
    if(!CryptGetHashParam(hash,HP_HASHVAL,out,&len,0)||len!=32) goto done;
    ok=1;
done:
    if(hash) CryptDestroyHash(hash);
    if(prov) CryptReleaseContext(prov,0);
    return ok;
}

static SOCKET connect_server(void) {
    WSADATA w; WSAStartup(0x202,&w);
    SOCKET s=socket(AF_INET,SOCK_STREAM,0);
    if(s==INVALID_SOCKET) return INVALID_SOCKET;
    struct sockaddr_in a; memset(&a,0,sizeof(a));
    a.sin_family=AF_INET; a.sin_port=htons(9000); inet_pton(AF_INET,"118.25.42.70",&a.sin_addr);
    if(connect(s,(struct sockaddr*)&a,sizeof(a))==SOCKET_ERROR){closesocket(s);return INVALID_SOCKET;}
    return s;
}

static int readn(SOCKET s, void *b, ec_u32 n){ ec_u32 g=0; while(g<n){int r=recv(s,(char*)b+g,n-g,0); if(r<1)return 0; g+=r;} return 1; }
static int frame(SOCKET s, ec_u8 op, const void *body, ec_u32 len, ec_u8 *st, ec_u8 **out, ec_u32 *outn){
    ec_u8 h[5]={op,(ec_u8)(len>>24),(ec_u8)(len>>16),(ec_u8)(len>>8),(ec_u8)len};
    if(send(s,(char*)h,5,0)!=5) return 0; if(len) send(s,(char*)body,len,0);
    if(!readn(s,h,5)) return 0; *st=h[0]; *outn=(ec_u32)h[1]<<24|h[2]<<16|h[3]<<8|h[4];
    *out=(ec_u8*)malloc(*outn?*outn:1); if(!*out) return 0; if(!readn(s,*out,*outn)){free(*out);*out=0;return 0;} return 1;
}
static int file_get(SOCKET s, const H h, ec_u8 **out, ec_u32 *n){ ec_u8 st; return frame(s,3,h,32,&st,out,n)&&st==0; }
static int children(SOCKET s, const H h, ec_u8 **out, ec_u32 *n){ ec_u8 st; return frame(s,5,h,32,&st,out,n)&&st==0&&*n>=4; }
static int user_get(SOCKET s, const H key, H val){ ec_u8 st,*out=0; ec_u32 n=0; ec_u8 body[64]; memcpy(body,user_id,32); memcpy(body+32,key,32); if(!frame(s,8,body,64,&st,&out,&n)) return 0; int ok=(st==0&&n>=32); if(ok) memcpy(val,out,32); free(out); return ok; }
static int first_child(SOCKET s, const H key, H child){ ec_u8 *out=0; ec_u32 n=0; if(!children(s,key,&out,&n)) return 0; ec_u32 count=(ec_u32)out[0]<<24|out[1]<<16|out[2]<<8|out[3]; int ok=count>0&&n>=36; if(ok) memcpy(child,out+4,32); free(out); return ok; }
static int resolve_key(SOCKET s, const H key, H hash){ if(user_get(s,key,hash)) return 1; return first_child(s,key,hash); }
static void hex32(const H h, char *out){ static const char*x="0123456789abcdef"; for(int i=0;i<32;i++){out[i*2]=x[h[i]>>4];out[i*2+1]=x[h[i]&15];} out[64]=0; }
static int same(const H a,const H b){return memcmp(a,b,32)==0;}
static int zero32(const ec_u8*p){for(int i=0;i<32;i++)if(p[i])return 0;return 1;}

static int reg_find_name(const char *s){ for(int i=0;i<regn;i++) if(_stricmp(regs[i].name,s)==0) return i; return -1; }
static int reg_find_token(const H h){ for(int i=0;i<regn;i++) if(same(regs[i].token,h)) return i; return -1; }
static void add_reg(const char *name, const H token){ if(regn>=512 || reg_find_name(name)>=0) return; strncpy(regs[regn].name,name,sizeof(regs[regn].name)-1); memcpy(regs[regn].token,token,32); regn++; }

static void scan_tag(SOCKET s, const H tag, const char *prefix, int depth) {
    if(depth>8) return;
    ec_u8 *ch=0,*raw=0; ec_u32 cn=0,rn=0;
    if(!children(s,tag,&ch,&cn)) return;
    ec_u32 count=(ec_u32)ch[0]<<24|ch[1]<<16|ch[2]<<8|ch[3];
    for(ec_u32 i=0;i<count;i++){
        if(4+i*40+32>cn) break;
        H child; memcpy(child,ch+4+i*40,32);
        raw=0; rn=0;
        if(!file_get(s,child,&raw,&rn)){ if(raw)free(raw); continue; }
        if(rn>0 && raw[0]=='#'){
            char name[160]; ec_u32 l=rn-1; if(l>90) l=90;
            memcpy(name,raw+1,l); name[l]=0;
            if(prefix && *prefix){ char tmp[160]; snprintf(tmp,sizeof(tmp),"%s/%s",prefix,name); scan_tag(s,child,tmp,depth+1); }
            else scan_tag(s,child,name,depth+1);
        } else {
            char name[96];
            if(prefix && *prefix) snprintf(name,sizeof(name),"%s",prefix);
            else { char hx[65]; hex32(child,hx); snprintf(name,sizeof(name),"token_%.*s",12,hx); }
            add_reg(name,child);
        }
        free(raw);
    }
    free(ch);
}

static void load_registry(void) {
    SOCKET s=connect_server();
    if(s==INVALID_SOCKET) return;
    H tag; sha256((const ec_u8*)"#TAG",4,tag);
    scan_tag(s,tag,"",0);
    closesocket(s);
}

static void load_first_view_key(void) {
    FILE *f=fopen("first_block.bin","rb");
    if(f){ fread(root_key,1,32,f); fclose(f); }
    f=fopen("id.bin","rb");
    if(f){ fread(user_id,1,32,f); fclose(f); }
}

static void draw_text(int x,int y,ec_u32 color,float size,const char*s){ dxgfx_draw_text(x,y,color,size,s,(ec_u32)strlen(s)); }
static void draw_hash(int x,int y,const H h){ char hx[65]; hex32(h,hx); dxgfx_draw_text(x,y,0xff8a8a9a,13.0f,hx,64); }

static void render_view(View *v) {
    dxgfx_draw_rect(v->x,v->y,v->w,v->h,0xff202838,1,1);
    dxgfx_draw_rect(v->x,v->y,v->w,v->h,0xff52607a,1,0);
    draw_text((int)v->x+10,(int)v->y+8,0xffe8e8f0,16,"block view");
    draw_hash((int)v->x+98,(int)v->y+10,v->key);
    SOCKET s=connect_server();
    if(s==INVALID_SOCKET){ draw_text((int)v->x+10,(int)v->y+36,0xffff8080,14,"server offline"); return; }
    H hash; ec_u8 *raw=0; ec_u32 n=0;
    if(resolve_key(s,v->key,hash)) file_get(s,hash,&raw,&n);
    closesocket(s);
    if(!raw){ draw_text((int)v->x+10,(int)v->y+36,0xffff8080,14,"block unavailable"); return; }
    unsigned off=0; int row=0;
    while(off+32<=n && !zero32(raw+off) && row<32){
        if(off+36>n) break;
        unsigned ps=*(unsigned*)(raw+off+32);
        char line[256]; int ri=reg_find_token(raw+off); char hx[65];
        if(ri>=0) snprintf(line,sizeof(line),"%04x  %s  payload=%u",off,regs[ri].name,ps);
        else { hex32(raw+off,hx); snprintf(line,sizeof(line),"%04x  %.16s  payload=%u",off,hx,ps); }
        ec_u32 c = (selected_view==0 && selected_offset==off) ? 0xffffd060 : 0xffd7d7df;
        draw_text((int)v->x+10,(int)v->y+34+row*20,c,14,line);
        off += 36 + ps; row++;
    }
    free(raw);
}

void ec_init(void) {
    if(inited) return;
    inited=1; zoom=1.0f;
    load_first_view_key();
    if(zero32(root_key)) memset(root_key,0,32);
    load_registry();
    viewn=1; selected_view=0; selected_offset=0;
    memcpy(views[0].key,root_key,32); views[0].x=40; views[0].y=70; views[0].w=900; views[0].h=610;
}

void ec_input(void) {
    ec_init();
    int m[4]={0}; dxgfx_mouse(m);
    float wx=(float)m[0]/zoom+cam_x, wy=(float)m[1]/zoom+cam_y;
    int pressed=(m[2]&1) && !(last_mouse[2]&1);
    if(pressed && viewn>0){
        View *v=&views[0];
        if(wx>=v->x && wx<=v->x+v->w && wy>=v->y+32 && wy<=v->y+v->h){
            int row=(int)((wy-(v->y+34))/20); if(row<0) row=0; selected_offset=(unsigned)row*36;
        }
    }
    int wheel=dxgfx_mouse_wheel(); if(wheel){ zoom += wheel*0.08f; if(zoom<0.2f)zoom=0.2f; if(zoom>4.0f)zoom=4.0f; }
    if(dxgfx_key_state(VK_ESCAPE,1)) should_halt=1;
    if(dxgfx_key_state(VK_BACK,1) && input_len>0) input[--input_len]=0;
    ec_u32 cp; while(dxgfx_text_input(&cp)){ if(cp>=32 && cp<127 && input_len<(int)sizeof(input)-1){ input[input_len++]=(char)cp; input[input_len]=0; } }
    memcpy(last_mouse,m,sizeof(m));
}

void ec_render(void) {
    ec_init();
    dxgfx_set_camera(cam_x,cam_y,zoom);
    if(viewn>0) render_view(&views[0]);
    char status[256]; snprintf(status,sizeof(status),"registry=%d  input=%s  Esc exits",regn,input);
    draw_text(18,18,0xffa8e6ff,16,status);
    int show=regn<10?regn:10;
    for(int i=0;i<show;i++) draw_text(960,70+i*20,0xffb8f0b8,14,regs[i].name);
}

void ec_frame(void) {
    ec_init();
    dxgfx_frame_begin();
    dxgfx_clear(0xff101018);
    ec_input();
    ec_render();
    dxgfx_frame_end();
    Sleep(16);
}

void ec_flush(void) { }
int ec_should_halt(void) { return should_halt || dxgfx_window_should_close(); }
int ec_registry_count(void) { ec_init(); return regn; }
int ec_registry_item(int idx, char *name, ec_u32 name_cap, ec_u8 token[32]) {
    ec_init(); if(idx<0||idx>=regn) return 0;
    if(name && name_cap){ strncpy(name,regs[idx].name,name_cap-1); name[name_cap-1]=0; }
    if(token) memcpy(token,regs[idx].token,32);
    return 1;
}
