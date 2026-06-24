#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#define MAX_NAMES 512
#define ROW_H 22
#define PAD_X 12
#define PAD_Y 4

typedef struct { H hash; char name[64]; } TokenEntry;

static TokenEntry g_names[MAX_NAMES];
static int g_name_count;
static int g_scroll;
static int g_select;
static HWND g_wnd;
static HFONT g_font;
static u8 *g_chain;
static u32 g_chain_len;
static u32 g_rec_count;
static u32 *g_rec_offs;
static int g_init;

static void load_names(void) {
    if (g_name_count) return;
    FILE *f = fopen("mods_map.txt", "rb");
    char line[256];
    if (!f) return;
    while (fgets(line, sizeof(line), f) && g_name_count < MAX_NAMES) {
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        size_t name_len = tab - line;
        if (name_len >= sizeof(g_names[0].name)) continue;
        memcpy(g_names[g_name_count].name, line, name_len);
        g_names[g_name_count].name[name_len] = 0;
        for (int i = 0; i < 32; i++) {
            unsigned int b;
            sscanf(tab + 1 + i * 2, "%2x", &b);
            g_names[g_name_count].hash[i] = (u8)b;
        }
        g_name_count++;
    }
    fclose(f);
}

static const char *name_for(const u8 *hash) {
    for (int i = 0; i < g_name_count; i++)
        if (memcmp(g_names[i].hash, hash, 32) == 0) return g_names[i].name;
    return 0;
}

static u32 parse_chain(void) {
    free(g_rec_offs);
    g_rec_offs = 0;
    g_rec_count = 0;
    if (!g_chain || g_chain_len < 36) return 0;
    g_rec_offs = (u32*)malloc(sizeof(u32) * 4096);
    u32 off = 0;
    while (off + 36 <= g_chain_len && g_rec_count < 4096) {
        int zero = 1;
        for (int i = 0; i < 32; i++) if (g_chain[off + i]) { zero = 0; break; }
        if (zero) break;
        u32 sp = (u32)g_chain[off + 32] | ((u32)g_chain[off + 33] << 8) |
                 ((u32)g_chain[off + 34] << 16) | ((u32)g_chain[off + 35] << 24);
        if (sp < 4 || off + 32 + sp > g_chain_len) break;
        g_rec_offs[g_rec_count++] = off;
        off += 32 + sp;
    }
    return g_rec_count;
}

static void draw(HDC dc, RECT r) {
    HBRUSH bg = CreateSolidBrush(RGB(18, 20, 28));
    FillRect(dc, &r, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, g_font);

    if (!g_rec_count) {
        SetTextColor(dc, RGB(148, 163, 184));
        TextOutA(dc, PAD_X, PAD_Y, "no records (empty or loading)", 29);
        return;
    }

    int visible = (r.bottom - r.top) / ROW_H;
    for (int i = 0; i < visible; i++) {
        int idx = g_scroll + i;
        if (idx < 0 || (u32)idx >= g_rec_count) break;

        u32 off = g_rec_offs[idx];
        u32 sp = (u32)g_chain[off + 32] | ((u32)g_chain[off + 33] << 8) |
                 ((u32)g_chain[off + 34] << 16) | ((u32)g_chain[off + 35] << 24);
        u32 plen = sp - 4;

        int y = PAD_Y + i * ROW_H;
        int sel = (u32)idx == (u32)g_select;

        if (sel) {
            RECT sr = {0, y, r.right, y + ROW_H};
            HBRUSH sb = CreateSolidBrush(RGB(38, 48, 72));
            FillRect(dc, &sr, sb);
            DeleteObject(sb);
        }

        char line[256];
        const char *nm = name_for(g_chain + off);
        if (nm)
            snprintf(line, sizeof(line), "%02d %-28s  +%u", idx, nm, plen);
        else
            snprintf(line, sizeof(line), "%02d %.8s...  +%u", idx, g_chain + off, plen);

        SetTextColor(dc, sel ? RGB(245, 247, 250) : RGB(148, 163, 184));
        TextOutA(dc, PAD_X, y, line, (int)strlen(line));

        if (plen && plen <= 64) {
            char payload[128];
            int pl = 0;
            for (u32 j = 0; j < plen && pl < (int)sizeof(payload) - 4; j++) {
                u8 b = g_chain[off + 36 + j];
                if (b >= 32 && b < 127) { payload[pl++] = (char)b; }
                else { pl += snprintf(payload + pl, sizeof(payload) - pl, "\\x%02x", b); }
            }
            payload[pl] = 0;
            SetTextColor(dc, RGB(116, 211, 194));
            TextOutA(dc, PAD_X + 420, y, payload, pl);
        }
    }
}

static LRESULT CALLBACK wnd_proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(w, &ps);
        RECT r; GetClientRect(w, &r);
        draw(dc, r);
        EndPaint(w, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEWHEEL: {
        int dz = (short)HIWORD(wp) / WHEEL_DELTA;
        g_scroll -= dz * 3;
        if (g_scroll < 0) g_scroll = 0;
        int max_scroll = (int)g_rec_count - 20;
        if (max_scroll < 0) max_scroll = 0;
        if (g_scroll > max_scroll) g_scroll = max_scroll;
        InvalidateRect(w, 0, 0);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int y = (int)(short)HIWORD(lp);
        int idx = g_scroll + y / ROW_H;
        if (idx >= 0 && (u32)idx < g_rec_count) g_select = idx;
        InvalidateRect(w, 0, 0);
        return 0;
    }
    case WM_SIZE:
        InvalidateRect(w, 0, 0);
        return 0;
    case WM_CLOSE:
        DestroyWindow(w);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (!s) { cnext(); return; }

    if (!g_init) {
        g_init = 1;
        load_names();

        WNDCLASSW wc = {0};
        wc.lpfnWndProc = wnd_proc;
        wc.hInstance = GetModuleHandleW(0);
        wc.lpszClassName = L"SimpleTokenEditor";
        wc.hCursor = LoadCursorW(0, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassW(&wc);

        g_wnd = CreateWindowW(L"SimpleTokenEditor", L"Token Editor",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,
            0, 0, GetModuleHandleW(0), 0);

        g_font = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Consolas");

        u32 len = 0;
        g_chain = block_read(s->view_hash, &len);
        if (g_chain) { g_chain_len = len; parse_chain(); }
    }

    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            free(g_chain); g_chain = 0;
            free(g_rec_offs); g_rec_offs = 0;
            if (g_font) DeleteObject(g_font);
            g_init = 0;
            cnext();
            return;
        }
    }
    cnext();
}
