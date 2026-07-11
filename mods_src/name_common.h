#ifndef NAME_COMMON_H
#define NAME_COMMON_H
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef struct { H token; char name[96]; } Entry;
static Entry g_names[2048];
static u32 g_name_n;
static int g_name_loaded;

static void name_load(void) {
    if (g_name_loaded) return;
    g_name_loaded = 1;
    const char *paths[] = { "instruction_names.bin", "./instruction_names.bin", ".\\instruction_names.bin", 0 };
    for (int p = 0; paths[p]; p++) {
        FILE *f = fopen(paths[p], "rb");
        if (!f) continue;
        fread(&g_name_n, 4, 1, f);
        if (g_name_n > 2048) g_name_n = 2048;
        g_name_n = (u32)fread(g_names, sizeof(Entry), g_name_n, f);
        fclose(f);
        return;
    }
}

/* query is prefix of name (case-insensitive, '_' ignored). Empty query -> no match. */
static int str_prefix_ci_us(const char *name, const char *query) {
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
    return !*b;
}
#endif
