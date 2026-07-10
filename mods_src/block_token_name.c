#include <stdio.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
typedef struct { H token; char name[96]; } Entry;
static Entry entries[2048]; static u32 entry_count; static int loaded;
static void load_index(void){if(loaded)return;loaded=1;FILE*f=fopen("instruction_names.bin","rb");if(!f)return;fread(&entry_count,4,1,f);if(entry_count>2048)entry_count=2048;entry_count=(u32)fread(entries,sizeof(Entry),entry_count,f);fclose(f);}
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){u32 o=*(u32*)pop(4);char out[96]={0};if(o+32<=cvm_cached_len()){u8*t=cvm_cached_base()+o;int nz=0;for(int i=0;i<32;i++)nz|=t[i];if(!nz)strcpy(out,"<end>");else{load_index();for(u32 i=0;i<entry_count;i++)if(!memcmp(entries[i].token,t,32)){strncpy(out,entries[i].name,95);break;}if(!out[0])snprintf(out,sizeof(out),"%02x%02x%02x%02x",t[0],t[1],t[2],t[3]);}}push(out,sizeof(out));cont();}
