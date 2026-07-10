#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u32 cvm_children(const H,H*,u32); extern __declspec(dllimport) u32 cvm_file_read(const H,u8*,u32);
#include "../uistate.h"
static int same(const H a,const H b){return !memcmp(a,b,32);} static int printable(u8*p,u32 n){for(u32 i=0;i<n;i++)if(p[i]<32||p[i]>126)return 0;return 1;}
static void add(UiState*s,const H h,const char*n,int tag){if(s->reg_count>=UI_MAX_REG)return;for(u32 i=0;i<s->reg_count;i++)if(same(s->reg[i].token,h))return;memcpy(s->reg[s->reg_count].token,h,32);strncpy(s->reg[s->reg_count].name,n,95);s->reg[s->reg_count].name[95]=0;s->reg[s->reg_count].tag=tag;s->reg_count++;}
static int child_name(const H h,char*out){H kids[64];u32 n=cvm_children(h,kids,64);if(n>64)n=64;for(u32 i=0;i<n;i++){u32 z=cvm_file_read(kids[i],(u8*)out,95);if(z>95)z=95;out[z]=0;if(z&&out[0]!='#'&&printable((u8*)out,z))return 1;}return 0;}
static void scan(UiState*s,const H p,int depth){if(depth>8)return;H kids[256];u32 n=cvm_children(p,kids,256);if(n>256)n=256;for(u32 i=0;i<n;i++){char name[96]={0};u32 z=cvm_file_read(kids[i],(u8*)name,95);if(z>95)z=95;name[z]=0;if(z&&name[0]=='#'){add(s,kids[i],name,1);scan(s,kids[i],depth+1);}else if(child_name(kids[i],name))add(s,kids[i],name,0);}}
__declspec(dllexport) void run(void){UiState*s=ui_state();if(!s->registry_ready){scan(s,s->registry_key,0);s->registry_ready=1;}cont();}
