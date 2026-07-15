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
    const char *paths[] = {
        "instruction_names.bin",
        ".\\instruction_names.bin",
        0
    };
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

/* query is prefix of name (case-insensitive, '_' ignored). Empty query -> no match. */
static int match(const char *name, const char *query) {
    if (!query || !*query) return 0;
    const char *a = name, *b = query;
    while (*a && *b) {
        char x = *a++, y = *b++;
        if (x == '_') { b--; continue; }
        if (y == '_') { a--; continue; }
        if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
        if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
        if (x != y) return 0;
    }
    return !*b; /* full query consumed as prefix of name */
}

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);

/* stack: text buffer (256 from var_read of input) -> token[32] (zero if no match) */
__declspec(dllexport) void run(void) {
    char *q = (char *)from(256);
    H out;
    memset(out, 0, 32);
    load_index();
    if (q && q[0]) {
        for (u32 i = 0; i < entry_count; i++) {
            if (match(entries[i].name, q)) {
                memcpy(out, entries[i].token, 32);
                break;
            }
        }
    }
    memcpy(slot(32), out, 32);
    cont();
}
