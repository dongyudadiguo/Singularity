#include <stdio.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);

typedef struct { H token; char name[96]; } Entry;
static Entry g_entries[2048];
static u32 g_entry_count;
static int g_loaded;
static void load_index(void){
    if (g_loaded) return; g_loaded=1;
    const char *paths[]={"instruction_names.bin","./instruction_names.bin",0};
    for (int p=0; paths[p]; p++){
        FILE *f=fopen(paths[p],"rb"); if(!f) continue;
        fread(&g_entry_count,4,1,f);
        if (g_entry_count>2048) g_entry_count=2048;
        g_entry_count=(u32)fread(g_entries,sizeof(Entry),g_entry_count,f);
        fclose(f); return;
    }
}
static int zero32(const u8 *p){ for(int i=0;i<32;i++) if(p[i]) return 0; return 1; }
static int same32(const u8 *a,const u8 *b){ return !memcmp(a,b,32); }
static const char *token_name(const u8 *tok){
    static char hex[12];
    if (zero32(tok)) return "<end>";
    load_index();
    for (u32 i=0;i<g_entry_count;i++) if (same32(g_entries[i].token,tok)) return g_entries[i].name;
    snprintf(hex,sizeof(hex),"%02x%02x%02x%02x",tok[0],tok[1],tok[2],tok[3]);
    return hex;
}
static int is_hash_carrier(const u8 *tok){
    const char *nm = token_name(tok);
    if (!nm || !nm[0] || nm[0]=='<') return 0;
    return !strcmp(nm,"cond_payload") || !strcmp(nm,"jump_payload")
        || !strcmp(nm,"exec_payload") || !strcmp(nm,"cond_reexec")
        || !strcmp(nm,"cond") || !strcmp(nm,"exec") || !strcmp(nm,"jump_payload");
}
/* stack: u32 row -> key[32] open target from CURRENT cached block */
__declspec(dllexport) void run(void){
    u32 row = *(u32*)pop(4);
    u8 *b = cvm_cached_base(); u32 nlen = cvm_cached_len();
    u32 o = 0;
    for (u32 r = 0; r < row && o + 36 <= nlen; r++) {
        if (zero32(b+o)) { o = nlen; break; }
        u32 pn = *(u32*)(b+o+32);
        if (o+36+pn > nlen) { o = nlen; break; }
        o += 36 + pn;
    }
    u8 key[32]; memset(key,0,32);
    if (o+32 <= nlen && !zero32(b+o)) {
        memcpy(key, b+o, 32);
        if (o+36 <= nlen) {
            u32 pn = *(u32*)(b+o+32);
            if (pn==32 && o+68<=nlen && is_hash_carrier(key)) {
                u8 ph[32]; memcpy(ph,b+o+36,32);
                if (!zero32(ph)) memcpy(key,ph,32);
            }
        }
    }
    push(key, 32);
    cont();
}
