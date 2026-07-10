#include <windows.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) int cvm_resolve_payload_hash(const H,H); extern __declspec(dllimport) u8*cvm_cached_base(void);extern __declspec(dllimport) u32 cvm_cached_len(void);extern __declspec(dllimport) void cvm_cached_set_len(u32);extern __declspec(dllimport) void cvm_cache_flush(void);extern __declspec(dllimport) int cvm_sha256(const u8*,u32,H);extern __declspec(dllimport) void cvm_edge(const H,const H);
#include "../uistate.h"
#include "../dxgfx.h"
#define MAX_BLOCK (1u<<20)
static int zero(u8*p){for(int i=0;i<32;i++)if(p[i])return 0;return 1;}static u32 sz(u8*p){return 36+*(u32*)(p+32);}static int valid(u8*b,u32 n,u32 o){return o+36<=n&&!zero(b+o)&&o+sz(b+o)<=n;}
static void load(UiState*s){H h;cvm_resolve_payload_hash(s->views[s->active_view].key,h);}
static void clear(UiState*s){s->input[0]=s->completion[0]=0;s->completion_index=0xffffffffu;}
static void insert(UiState*s,const H tok,const u8*p,u32 pn){u8*b=cvm_cached_base();u32 n=cvm_cached_len(),o=s->point_off;if(n<32||o>n-32||n+36+pn>MAX_BLOCK)return;memmove(b+o+36+pn,b+o,n-o);memcpy(b+o,tok,32);*(u32*)(b+o+32)=pn;if(pn)memcpy(b+o+36,p,pn);cvm_cached_set_len(n+36+pn);s->point_off=o+36+pn;s->dirty=1;}
static void del(UiState*s){u8*b=cvm_cached_base();u32 n=cvm_cached_len(),o=s->point_off;if(!valid(b,n,o))return;u32 z=sz(b+o);memmove(b+o,b+o+z,n-o-z);cvm_cached_set_len(n-z);s->dirty=1;}
static void child(UiState*s){H k;u8 z[32]={0};char name[96];if(s->input[0])strncpy(name,s->input,95);else wsprintfA(name,"block%u",s->view_count);name[95]=0;cvm_sha256((u8*)name,(u32)strlen(name),k);cvm_edge(k,z);insert(s,k,0,0);if(s->view_count<UI_MAX_VIEWS){UiView*v=&s->views[s->view_count++];memset(v,0,sizeof(*v));v->used=1;memcpy(v->key,k,32);v->x=s->views[s->active_view].x+380;v->y=s->views[s->active_view].y;v->linked=1;v->parent=(int)s->active_view;v->link_x=80;v->link_y=20;}clear(s);}
__declspec(dllexport) void run(void){UiState*s=ui_state();u8*d=s->keys_down,*p=s->keys_pressed,*r=s->keys_released;int*m=s->mouse;load(s);if(p[VK_SPACE]||p[VK_TAB]){if(s->completion_index!=0xffffffffu){insert(s,s->reg[s->completion_index].token,0,0);clear(s);}}if(p[VK_MENU])child(s);if(p[VK_OEM_3]&&s->input[0]){H tok;cvm_sha256((u8*)"data",4,tok);insert(s,tok,(u8*)s->input,(u32)strlen(s->input));clear(s);}if(p[VK_DELETE])del(s);if(p[VK_UP]||p[VK_LEFT]){u8*b=cvm_cached_base();u32 o=0,last=0;while(valid(b,cvm_cached_len(),o)&&o<s->point_off){last=o;o+=sz(b+o);}s->point_off=last;}if(p[VK_DOWN]||p[VK_RIGHT]){u8*b=cvm_cached_base();if(valid(b,cvm_cached_len(),s->point_off))s->point_off+=sz(b+s->point_off);}if(d[VK_CONTROL]&&p['S']){cvm_cache_flush();s->dirty=0;}cont();}
