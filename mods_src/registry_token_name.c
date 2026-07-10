#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef struct { H token; char name[96]; } Entry;
static Entry entries[2048];
static u32 entry_count;
static int loaded;

static void load_index(void) {
    if (loaded) return;
    loaded = 1;
    const char *paths[] = { "instruction_names.bin", ".\\instruction_names.bin", 0 };
    for (int p = 0; paths[p]; p++) {
        FILE *f = fopen(paths[p], "rb");
        if (!f) continue;
        fread(&entry_count, 4, 1, f);
        if (entry_count > 2048) entry_count = 2048;
        entry_count = (u32)fread(entries, sizeof(Entry), entry_count, f);
        fclose(f);
        return;
    }
}

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void *, u32);

__declspec(dllexport) void run(void) {
    H token;
    memcpy(token, pop(32), 32);
    char out[96];
    memset(out, 0, sizeof(out));
    int nz = 0;
    for (int i = 0; i < 32; i++) nz |= token[i];
    if (nz) {
        load_index();
        for (u32 i = 0; i < entry_count; i++) {
            if (!memcmp(entries[i].token, token, 32)) {
                strncpy(out, entries[i].name, 95);
                break;
            }
        }
        if (!out[0])
            snprintf(out, sizeof(out), "%02x%02x%02x%02x", token[0], token[1], token[2], token[3]);
    }
    push(out, sizeof(out));
    cont();
}
