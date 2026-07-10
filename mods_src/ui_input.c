#include <windows.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
#include "../uistate.h"
#include "../dxgfx.h"
static int command(u8*p){return p[VK_SPACE]||p[VK_TAB]||p[VK_MENU]||p[VK_OEM_3]||p[VK_DELETE]||p[VK_INSERT]||p[VK_BACK]||p[VK_ESCAPE]||p[VK_RETURN];}
static int match(const char*n,const char*i){if(!*i)return 0;while(*n&&*i){char a=*n++,b=*i++;if(a=='_'){i--;continue;}if(b=='_'){n--;continue;}if(a>='A'&&a<='Z')a+=32;if(b>='A'&&b<='Z')b+=32;if(a!=b)return 0;}return !*i;}
static void complete(UiState*s){s->completion[0]=0;s->completion_index=0xffffffffu;for(u32 i=0;i<s->reg_count;i++)if(!s->reg[i].tag&&match(s->reg[i].name,s->input)){strncpy(s->completion,s->reg[i].name,95);s->completion_index=i;return;}}
__declspec(dllexport) void run(void){UiState*s=ui_state();u8*down=s->keys_down,*pressed=s->keys_pressed,*released=s->keys_released;int*m=s->mouse;char*text=s->text;memset(text,0,64);dxgfx_input_snapshot(down,pressed,released,m,text);if(pressed[VK_ESCAPE])s->input[0]=0;size_t l=strlen(s->input);if(text[0]&&!command(pressed)&&l<255)strncat(s->input,text,255-l);l=strlen(s->input);if(pressed[VK_BACK]&&l)s->input[l-1]=0;complete(s);if(m[5]){s->zoom+=(float)m[5]*.1f*s->zoom;if(s->zoom<.15f)s->zoom=.15f;if(s->zoom>6)s->zoom=6;}if(m[2]&4){s->cam_x-=((float)m[0]-s->last_mx)/s->zoom;s->cam_y-=((float)m[1]-s->last_my)/s->zoom;}if(!(m[2]&2))s->dragging_view=-1;if((m[2]&2)&&s->dragging_view>=0&&(u32)s->dragging_view<s->view_count){s->views[s->dragging_view].x+=((float)m[0]-s->last_mx)/s->zoom;s->views[s->dragging_view].y+=((float)m[1]-s->last_my)/s->zoom;}s->last_mx=(float)m[0];s->last_my=(float)m[1];cont();}
