#include <stdio.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
typedef struct { H token; char name[96]; } Entry;
static Entry entries[2048]; static u32 entry_count; static int loaded;
static void load_index(void){if(loaded)return;loaded=1;FILE*f=fopen("instruction_names.bin","rb");if(!f)return;fread(&entry_count,4,1,f);if(entry_count>2048)entry_count=2048;entry_count=(u32)fread(entries,sizeof(Entry),entry_count,f);fclose(f);}
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
static int match(const char*a,const char*b){if(!*b)return 0;while(*a&&*b){char x=*a++,y=*b++;if(x=='_'){b--;continue;}if(y=='_'){a--;continue;}if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}return !*b;}
__declspec(dllexport) void run(void){char*q=pop(256);H out={0};load_index();for(u32 i=0;i<entry_count;i++)if(match(entries[i].name,q)){memcpy(out,entries[i].token,32);break;}push(out,32);cont();}
