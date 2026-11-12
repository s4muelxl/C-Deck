#include "interface.h"
#include "actions.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>

#define IDI_APP_ICON        101
#define CLASS_NAME          L"CDeckMainWindow"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
typedef HRESULT (WINAPI *pfnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

// Modern C-Deck Color Palette
#define COLOR_BG            RGB(21, 23, 27)     // #15171B (Deep canvas background)
#define COLOR_TOPBAR        RGB(18, 20, 24)     // #121418 (Header top bar)
#define COLOR_SIDEBAR       RGB(22, 24, 28)     // #16181C (Left sidebar)
#define COLOR_INSPECTOR     RGB(24, 26, 31)     // #181A1F (Right property inspector)
#define COLOR_KEY_BASE      RGB(25, 28, 35)     // #191C23 (Keycap face)
#define COLOR_KEY_PRESSED   RGB(15, 17, 22)     // #0F1116 (Key pressed)
#define COLOR_BORDER_DARK   RGB(38, 41, 50)     // #262932 (Separators / panel borders)
#define COLOR_BORDER_KEY    RGB(43, 48, 60)     // #2B303C (Normal key outline)
#define COLOR_BORDER_BTN    RGB(56, 63, 77)     // #383F4D (Button borders)

// Brand Vibrant Lime Accent & Glows
#define COLOR_ACCENT_LIME   RGB(157, 255, 0)    // #9DFF00 (Official brand lime green)
#define COLOR_ACCENT_BG     RGB(24, 40, 26)     // #18281A (Active sidebar pill background)
#define COLOR_MUTE_AMBER    RGB(255, 168, 52)   // #FFA834 (Mute amber/orange icon)
#define COLOR_WHITE         RGB(255, 255, 255)  // #FFFFFF
#define COLOR_TEXT_MUTED    RGB(158, 172, 184)  // #9EACB8
#define COLOR_TEXT_DIM      RGB(126, 135, 148)  // #7E8794
#define COLOR_INPUT_BG      RGB(22, 25, 31)     // #16191F

// Control IDs
#define ID_SETTINGS         1002
#define ID_PROFILE_BTN      1003
#define ID_STATUS           1099

// 15 Keys in Canvas (5 columns x 3 rows)
#define ID_KEY_BASE         1100
#define ID_KEY_COUNT        15

// Property Inspector Controls
#define ID_INSP_TITLE       1201
#define ID_INSP_PATH        1202
#define ID_INSP_BROWSE_PATH 1203
#define ID_INSP_TEST        1204
#define ID_INSP_SAVE        1205
#define ID_INSP_REMOVE      1206
#define ID_INSP_VOL_TRACK   1207
#define ID_INSP_MUTE_BTN    1208

// Action Library Categories
#define ID_LIB_FAVORITES    1301
#define ID_LIB_SYSTEM       1302
#define ID_LIB_AUDIO        1303
#define ID_LIB_MEDIA        1304
#define ID_LIB_APPS         1305
#define ID_LIB_NAVIGATION   1306
#define ID_LIB_MULTIACTION  1307

// Tray and Hotkeys
#define ID_TRAY_OPEN        3001
#define ID_TRAY_EXIT        3002
#define ID_HOTKEY_UP        1
#define ID_HOTKEY_DOWN      2
#define ID_HOTKEY_MUTE      3
#define WM_TRAYICON         (WM_APP + 1)
#define ID_VOLUME_TIMER     1

// Global application state
static DeckConfig g_config;
static char g_config_path[MAX_PATH];
static HWND g_main_window;
static HWND g_status;
static HWND g_keys[ID_KEY_COUNT];
static int g_selected_key = 8; // Default to Key 8 (Paint)
static int g_selected_category = ID_LIB_SYSTEM;

// Inspector controls
static HWND g_insp_title_edit;
static HWND g_insp_path_edit;
static HWND g_insp_path_browse;
static HWND g_insp_test_btn;
static HWND g_insp_save_btn;
static HWND g_insp_remove_btn;
static HWND g_insp_vol_track;
static HWND g_insp_mute_btn;

// Fonts and Brushes
static HFONT g_font_title;
static HFONT g_font_header;
static HFONT g_font_label;
static HFONT g_font_sub;
static HFONT g_font_btn;
static HBRUSH g_brush_bg;
static HBRUSH g_brush_panel;
static HBRUSH g_brush_input;

static NOTIFYICONDATAW g_tray_data;
static int g_tray_added;
static UINT g_wm_close_msg;

// Forward declarations
static void layout_controls(HWND hwnd);
static void update_inspector_ui(void);
static void draw_key(const DRAWITEMSTRUCT *draw);
static void draw_sidebar_button(const DRAWITEMSTRUCT *draw);
static void draw_profile_button(const DRAWITEMSTRUCT *draw);
static void draw_settings_button(const DRAWITEMSTRUCT *draw);
static void draw_inspector_button(const DRAWITEMSTRUCT *draw, const wchar_t *text);
static void draw_browse_button(const DRAWITEMSTRUCT *draw);
static void draw_inspector_preview(HDC dc, RECT rect);
static const wchar_t *get_key_default_title(int idx);

static void utf8_to_wide(const char *utf8, wchar_t *wide, int max_chars) {
    if (!utf8 || !wide || max_chars <= 0) return;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, max_chars);
    wide[max_chars - 1] = L'\0';
}

static void wide_to_utf8(const wchar_t *wide, char *utf8, int max_bytes) {
    if (!wide || !utf8 || max_bytes <= 0) return;
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, max_bytes, NULL, NULL);
    utf8[max_bytes - 1] = '\0';
}

static int build_config_path(char *out, DWORD capacity) {
    DWORD length = GetModuleFileNameA(NULL, out, capacity);
    char *separator;
    const char suffix[] = "config\\apps.cfg";
    if (length == 0 || length >= capacity) return 0;
    separator = strrchr(out, '\\');
    if (!separator) separator = strrchr(out, '/');
    if (!separator) return 0;
    separator[1] = '\0';
    if (strlen(out) + sizeof(suffix) > capacity) return 0;
    strcat(out, suffix);
    return 1;
}

static void set_status(const wchar_t *message) {
    if (g_status) SetWindowTextW(g_status, message);
}

static void report_result(ActionResult result, const wchar_t *success_message) {
    switch (result) {
        case ACTION_SUCCESS: set_status(success_message); break;
        case ACTION_AUDIO_UNAVAILABLE: set_status(L"Dispositivo de áudio indisponível."); break;
        case ACTION_MEDIA_UNAVAILABLE: set_status(L"O Windows não aceitou o comando de mídia."); break;
        case ACTION_LAUNCH_FAILED: set_status(L"Não foi possível abrir o aplicativo. Confira o caminho."); break;
        case ACTION_NOT_CONFIGURED: set_status(L"Atalho não configurado. Preencha os campos ao lado."); break;
        default: set_status(L"Não foi possível executar esta ação."); break;
    }
}

static void enable_dark_titlebar(HWND hwnd) {
    HMODULE dwm = LoadLibraryA("dwmapi.dll");
    if (dwm) {
        pfnDwmSetWindowAttribute set_attr = (pfnDwmSetWindowAttribute)GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (set_attr) {
            BOOL enable = TRUE;
            if (FAILED(set_attr(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &enable, sizeof(enable)))) {
                set_attr(hwnd, 19, &enable, sizeof(enable));
            }
        }
        FreeLibrary(dwm);
    }
}

static int add_tray_icon(HWND hwnd) {
    if (g_tray_added) return 1;
    ZeroMemory(&g_tray_data, sizeof(g_tray_data));
    g_tray_data.cbSize = sizeof(g_tray_data);
    g_tray_data.hWnd = hwnd;
    g_tray_data.uID = 1;
    g_tray_data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray_data.uCallbackMessage = WM_TRAYICON;
    g_tray_data.hIcon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(IDI_APP_ICON));
    if (!g_tray_data.hIcon) g_tray_data.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    lstrcpynW(g_tray_data.szTip, L"C-Deck — Elgato Stream Deck Edition", (int)(sizeof(g_tray_data.szTip) / sizeof(wchar_t)));
    g_tray_added = Shell_NotifyIconW(NIM_ADD, &g_tray_data) ? 1 : 0;
    return g_tray_added;
}

static void remove_tray_icon(void) {
    if (g_tray_added) Shell_NotifyIconW(NIM_DELETE, &g_tray_data);
    g_tray_added = 0;
}

static void show_tray_menu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, L"Abrir C-Deck");
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Sair");
    SetForegroundWindow(hwnd);
    {
        POINT point;
        UINT command;
        GetCursorPos(&point);
        command = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, 0, hwnd, NULL);
        PostMessageW(hwnd, WM_NULL, 0, 0);
        if (command) SendMessageW(hwnd, WM_COMMAND, command, 0);
    }
    DestroyMenu(menu);
}

// Key Action Execution
static void execute_key_action(int key_index) {
    switch (key_index) {
        case 0: // Volume +
            report_result(action_volume_step(0.05f), L"Volume aumentado em +5%.");
            break;
        case 1: // Mudo
            report_result(action_toggle_mute(), L"Mudo alternado com sucesso.");
            break;
        case 2: // Play / Pause
            report_result(action_media_play_pause(), L"Play / Pause enviado ao Windows.");
            break;
        case 3: // Faixa Anterior
            report_result(action_media_previous(), L"Comando de faixa anterior enviado.");
            break;
        case 4: // Proxima Faixa
            report_result(action_media_next(), L"Comando de próxima faixa enviado.");
            break;
        case 5: // Volume -
            report_result(action_volume_step(-0.05f), L"Volume reduzido em −5%.");
            break;
        case 6: // App Slot 1 (Bloco de Notas)
            report_result(action_launch_app(&g_config.apps[0]), L"Atalho executado.");
            break;
        case 7: // App Slot 2 (Calculadora)
            report_result(action_launch_app(&g_config.apps[1]), L"Atalho executado.");
            break;
        case 8: // App Slot 3 (Paint)
            report_result(action_launch_app(&g_config.apps[2]), L"Atalho executado.");
            break;
        case 9: // Pasta
            ShellExecuteA(NULL, "open", "explorer.exe", NULL, NULL, SW_SHOWNORMAL);
            set_status(L"Abrindo Explorador de Arquivos.");
            break;
        default:
            set_status(L"Slot vazio selecionado. Configure ao lado.");
            break;
    }
    update_inspector_ui();
}

static void browse_file_for_inspector(HWND hwnd) {
    OPENFILENAMEW dialog;
    wchar_t selected[MAX_PATH];
    const wchar_t *filter = L"Aplicativos (*.exe;*.com;*.bat;*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Todos os arquivos (*.*)\0*.*\0\0";
    ZeroMemory(&dialog, sizeof(dialog));
    ZeroMemory(selected, sizeof(selected));
    if (g_insp_path_edit) GetWindowTextW(g_insp_path_edit, selected, MAX_PATH);

    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = selected;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = L"Escolher aplicativo (.exe)";
    if (GetOpenFileNameW(&dialog)) {
        if (g_insp_path_edit) SetWindowTextW(g_insp_path_edit, selected);
    }
}

static void save_inspector_changes(void) {
    if (g_selected_key < 6 || g_selected_key > 8) return;
    int app_idx = g_selected_key - 6;
    wchar_t wname[DECK_TEXT_MAX];
    wchar_t wpath[DECK_TEXT_MAX];
    char name[DECK_TEXT_MAX];
    char path[DECK_TEXT_MAX];

    GetWindowTextW(g_insp_title_edit, wname, DECK_TEXT_MAX);
    GetWindowTextW(g_insp_path_edit, wpath, DECK_TEXT_MAX);

    wide_to_utf8(wname, name, DECK_TEXT_MAX);
    wide_to_utf8(wpath, path, DECK_TEXT_MAX);

    // Trim whitespace
    char *s = name; while (*s && isspace((unsigned char)*s)) ++s; memmove(name, s, strlen(s) + 1);
    char *e = name + strlen(name); while (e > name && isspace((unsigned char)e[-1])) *--e = '\0';

    s = path; while (*s && isspace((unsigned char)*s)) ++s; memmove(path, s, strlen(s) + 1);
    e = path + strlen(path); while (e > path && isspace((unsigned char)e[-1])) *--e = '\0';

    strcpy(g_config.apps[app_idx].name, name);
    strcpy(g_config.apps[app_idx].path, path);
    g_config.apps[app_idx].configured = (name[0] != '\0' && path[0] != '\0');

    if (config_save(g_config_path, &g_config)) {
        set_status(L"Atalho salvo com sucesso.");
    } else {
        set_status(L"Erro ao salvar configuração.");
    }

    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
}

static void remove_selected_action(void) {
    if (g_selected_key < 6 || g_selected_key > 8) return;
    int app_idx = g_selected_key - 6;
    ZeroMemory(&g_config.apps[app_idx], sizeof(DeckApp));
    config_save(g_config_path, &g_config);
    update_inspector_ui();
    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
    set_status(L"Ação removida do slot.");
}

static void update_inspector_ui(void) {
    if (!g_insp_title_edit) return;

    if (g_selected_key >= 6 && g_selected_key <= 8) {
        int app_idx = g_selected_key - 6;
        const DeckApp *app = &g_config.apps[app_idx];
        ShowWindow(g_insp_title_edit, SW_SHOW);
        ShowWindow(g_insp_path_edit, SW_SHOW);
        ShowWindow(g_insp_path_browse, SW_SHOW);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_save_btn, SW_SHOW);
        ShowWindow(g_insp_remove_btn, SW_SHOW);
        ShowWindow(g_insp_vol_track, SW_HIDE);
        ShowWindow(g_insp_mute_btn, SW_HIDE);

        wchar_t wname[DECK_TEXT_MAX];
        wchar_t wpath[DECK_TEXT_MAX];
        utf8_to_wide(app->name, wname, DECK_TEXT_MAX);
        utf8_to_wide(app->path, wpath, DECK_TEXT_MAX);

        SetWindowTextW(g_insp_title_edit, wname);
        SetWindowTextW(g_insp_path_edit, wpath);
    } else if (g_selected_key == 0 || g_selected_key == 1 || g_selected_key == 5) {
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_vol_track, SW_SHOW);
        ShowWindow(g_insp_mute_btn, SW_HIDE);

        float level = 0.0f; int muted = 0;
        action_get_volume(&level, &muted);
        SendMessageW(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
    } else if (g_selected_key == 2) {
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_HIDE);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_vol_track, SW_SHOW);
        ShowWindow(g_insp_mute_btn, SW_SHOW);

        float level = 0.0f; int muted = 0;
        action_get_volume(&level, &muted);
        SendMessageW(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
    } else {
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_vol_track, SW_HIDE);
        ShowWindow(g_insp_mute_btn, SW_HIDE);
    }

    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
}

static void layout_controls(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width < 600 || height < 400) return;

    int topbar_h = 52;
    int statusbar_h = 28;
    int sidebar_w = 200;
    int inspector_w = 320;
    int canvas_x = sidebar_w;
    int canvas_w = width - sidebar_w - inspector_w;
    int content_h = height - topbar_h - statusbar_h;

    // Top Bar Controls
    HWND profile_btn = GetDlgItem(hwnd, ID_PROFILE_BTN);
    if (profile_btn) MoveWindow(profile_btn, 172, 10, 150, 32, TRUE);

    HWND settings_btn = GetDlgItem(hwnd, ID_SETTINGS);
    if (settings_btn) MoveWindow(settings_btn, width - 120, 10, 104, 32, TRUE);

    // Sidebar Category Buttons (7 categories, start at y=68)
    int side_top = topbar_h + 16;
    for (int i = 0; i < 7; ++i) {
        HWND btn = GetDlgItem(hwnd, ID_LIB_FAVORITES + i);
        if (btn) MoveWindow(btn, 12, side_top + i * 42, sidebar_w - 24, 36, TRUE);
    }

    // 15 Keys in Canvas (5 cols x 3 rows)
    int grid_cols = 5;
    int grid_rows = 3;
    int gap = 14;
    int max_key_w = (canvas_w - 60 - (grid_cols - 1) * gap) / grid_cols;
    int max_key_h = (content_h - 100 - (grid_rows - 1) * gap) / grid_rows;
    int key_size = max_key_w < max_key_h ? max_key_w : max_key_h;
    if (key_size > 108) key_size = 108;
    if (key_size < 64) key_size = 64;

    int grid_total_w = grid_cols * key_size + (grid_cols - 1) * gap;
    int grid_total_h = grid_rows * key_size + (grid_rows - 1) * gap;
    int grid_start_x = canvas_x + (canvas_w - grid_total_w) / 2;
    int grid_start_y = topbar_h + 60 + (content_h - 90 - grid_total_h) / 2;

    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            int idx = row * grid_cols + col;
            int kx = grid_start_x + col * (key_size + gap);
            int ky = grid_start_y + row * (key_size + gap);
            if (g_keys[idx]) MoveWindow(g_keys[idx], kx, ky, key_size, key_size, TRUE);
        }
    }

    // Property Inspector Controls
    int insp_x = width - inspector_w + 22;
    int insp_w = inspector_w - 44;
    int iy = topbar_h + 80;

    if (g_insp_title_edit) MoveWindow(g_insp_title_edit, insp_x, iy, insp_w, 28, TRUE);
    iy += 54;
    if (g_insp_path_edit) MoveWindow(g_insp_path_edit, insp_x, iy, insp_w - 38, 28, TRUE);
    if (g_insp_path_browse) MoveWindow(g_insp_path_browse, insp_x + insp_w - 32, iy, 32, 28, TRUE);
    iy += 44;
    if (g_insp_test_btn) MoveWindow(g_insp_test_btn, insp_x, iy, insp_w, 34, TRUE);
    iy += 42;
    if (g_insp_save_btn) MoveWindow(g_insp_save_btn, insp_x, iy, insp_w, 34, TRUE);
    iy += 42;
    if (g_insp_remove_btn) MoveWindow(g_insp_remove_btn, insp_x, iy, insp_w, 34, TRUE);

    // Audio controls
    if (g_insp_vol_track) MoveWindow(g_insp_vol_track, insp_x, topbar_h + 80, insp_w, 28, TRUE);
    if (g_insp_mute_btn) MoveWindow(g_insp_mute_btn, insp_x, topbar_h + 120, insp_w, 34, TRUE);

    // Status bar
    if (g_status) MoveWindow(g_status, 16, height - statusbar_h + 4, width - 260, 20, TRUE);
}

// Draw the official faceted C-Deck logo from brand specification
static void draw_logo(HDC dc, int ox, int oy) {
    // 1. Faceted White / Silver C ribbon
    POINT ribbon_pts[] = {
        { ox + 21, oy + 1 },
        { ox + 9,  oy + 1 },
        { ox + 1,  oy + 8 },
        { ox + 1,  oy + 15 },
        { ox + 9,  oy + 23 },
        { ox + 21, oy + 23 },
        { ox + 21, oy + 18 },
        { ox + 11, oy + 18 },
        { ox + 6,  oy + 14 },
        { ox + 6,  oy + 10 },
        { ox + 11, oy + 6 },
        { ox + 21, oy + 6 }
    };
    HBRUSH white_brush = CreateSolidBrush(COLOR_WHITE);
    HPEN null_pen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ old_b = SelectObject(dc, white_brush);
    HGDIOBJ old_p = SelectObject(dc, null_pen);
    Polygon(dc, ribbon_pts, 12);

    // Subtle 3D shadow on bottom curve of C ribbon
    POINT shadow_pts[] = {
        { ox + 1,  oy + 15 },
        { ox + 9,  oy + 23 },
        { ox + 21, oy + 23 },
        { ox + 21, oy + 18 },
        { ox + 11, oy + 18 },
        { ox + 6,  oy + 14 }
    };
    HBRUSH shadow_brush = CreateSolidBrush(RGB(185, 192, 204));
    SelectObject(dc, shadow_brush);
    Polygon(dc, shadow_pts, 6);
    DeleteObject(shadow_brush);

    // 2. Lime Green Accent Wedge at lower right
    POINT lime_pts[] = {
        { ox + 15, oy + 23 },
        { ox + 22, oy + 23 },
        { ox + 28, oy + 18 },
        { ox + 21, oy + 18 }
    };
    HBRUSH lime_brush = CreateSolidBrush(COLOR_ACCENT_LIME);
    SelectObject(dc, lime_brush);
    Polygon(dc, lime_pts, 4);
    DeleteObject(lime_brush);

    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(white_brush);
    DeleteObject(null_pen);
}

// Glowing Lime Selection Border for active key and preview
static void draw_lime_glow_border(HDC dc, RECT rc, int radius) {
    HPEN glow_pen1 = CreatePen(PS_SOLID, 1, RGB(42, 75, 14));
    HGDIOBJ old_p = SelectObject(dc, glow_pen1);
    HGDIOBJ old_b = SelectObject(dc, GetStockObject(NULL_BRUSH));
    RoundRect(dc, rc.left - 2, rc.top - 2, rc.right + 2, rc.bottom + 2, radius + 2, radius + 2);

    HPEN glow_pen2 = CreatePen(PS_SOLID, 1, RGB(85, 150, 18));
    SelectObject(dc, glow_pen2);
    RoundRect(dc, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1, radius + 1, radius + 1);

    HPEN lime_pen = CreatePen(PS_SOLID, 2, COLOR_ACCENT_LIME);
    SelectObject(dc, lime_pen);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(glow_pen1);
    DeleteObject(glow_pen2);
    DeleteObject(lime_pen);
}

// Vector Category Icons (Left Sidebar)
static void draw_category_icon(HDC dc, int id, int cx, int cy, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old_p = SelectObject(dc, pen);
    HGDIOBJ old_b = SelectObject(dc, brush);

    switch (id) {
        case ID_LIB_FAVORITES: { // Star outline
            POINT star[10] = {
                { cx, cy - 7 },
                { cx + 2, cy - 2 },
                { cx + 7, cy - 2 },
                { cx + 3, cy + 2 },
                { cx + 4, cy + 7 },
                { cx, cy + 4 },
                { cx - 4, cy + 7 },
                { cx - 3, cy + 2 },
                { cx - 7, cy - 2 },
                { cx - 2, cy - 2 }
            };
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            Polygon(dc, star, 10);
            break;
        }
        case ID_LIB_SYSTEM: { // Gear outline
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            Ellipse(dc, cx - 6, cy - 6, cx + 6, cy + 6);
            Ellipse(dc, cx - 2, cy - 2, cx + 2, cy + 2);
            // 6 Gear teeth
            for (int a = 0; a < 6; ++a) {
                int dx = (a == 0 || a == 3) ? 0 : ((a == 1 || a == 2) ? 6 : -6);
                int dy = (a == 0) ? -7 : ((a == 3) ? 7 : ((a == 1 || a == 5) ? -4 : 4));
                MoveToEx(dc, cx + dx / 2, cy + dy / 2, NULL);
                LineTo(dc, cx + dx, cy + dy);
            }
            break;
        }
        case ID_LIB_AUDIO: { // Equalizer waves
            MoveToEx(dc, cx - 6, cy - 3, NULL); LineTo(dc, cx - 6, cy + 4);
            MoveToEx(dc, cx - 3, cy - 6, NULL); LineTo(dc, cx - 3, cy + 7);
            MoveToEx(dc, cx,     cy - 8, NULL); LineTo(dc, cx,     cy + 9);
            MoveToEx(dc, cx + 3, cy - 5, NULL); LineTo(dc, cx + 3, cy + 6);
            MoveToEx(dc, cx + 6, cy - 2, NULL); LineTo(dc, cx + 6, cy + 3);
            break;
        }
        case ID_LIB_MEDIA: { // Play / Pause
            POINT play_pts[3] = { { cx - 7, cy - 5 }, { cx - 1, cy }, { cx - 7, cy + 5 } };
            Polygon(dc, play_pts, 3);
            RECT b1 = { cx + 2, cy - 5, cx + 4, cy + 6 };
            RECT b2 = { cx + 5, cy - 5, cx + 7, cy + 6 };
            FillRect(dc, &b1, brush);
            FillRect(dc, &b2, brush);
            break;
        }
        case ID_LIB_APPS: { // 4-square grid
            RECT q1 = { cx - 6, cy - 6, cx - 1, cy - 1 };
            RECT q2 = { cx + 1, cy - 6, cx + 6, cy - 1 };
            RECT q3 = { cx - 6, cy + 1, cx - 1, cy + 6 };
            RECT q4 = { cx + 1, cy + 1, cx + 6, cy + 6 };
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            RoundRect(dc, q1.left, q1.top, q1.right, q1.bottom, 2, 2);
            RoundRect(dc, q2.left, q2.top, q2.right, q2.bottom, 2, 2);
            RoundRect(dc, q3.left, q3.top, q3.right, q3.bottom, 2, 2);
            RoundRect(dc, q4.left, q4.top, q4.right, q4.bottom, 2, 2);
            break;
        }
        case ID_LIB_NAVIGATION: { // Compass
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            Ellipse(dc, cx - 7, cy - 7, cx + 7, cy + 7);
            POINT needle1[3] = { { cx, cy }, { cx + 4, cy - 4 }, { cx - 1, cy - 3 } };
            POINT needle2[3] = { { cx, cy }, { cx - 4, cy + 4 }, { cx + 1, cy + 3 } };
            SelectObject(dc, brush);
            Polygon(dc, needle1, 3);
            Polygon(dc, needle2, 3);
            break;
        }
        case ID_LIB_MULTIACTION: { // Layered squares
            SelectObject(dc, GetStockObject(NULL_BRUSH));
            RoundRect(dc, cx - 6, cy - 3, cx + 2, cy + 6, 2, 2);
            RoundRect(dc, cx - 2, cy - 6, cx + 6, cy + 2, 2, 2);
            break;
        }
    }

    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(pen);
    DeleteObject(brush);
}

// Vector Key Icons (15 Keys Grid & Inspector Preview)
static void draw_key_icon(HDC dc, int idx, int cx, int cy, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old_p = SelectObject(dc, pen);
    HGDIOBJ old_b = SelectObject(dc, brush);

    switch (idx) {
        case 0: { // Volume +
            POINT cone[6] = {
                { cx - 10, cy - 4 }, { cx - 6, cy - 4 }, { cx - 1, cy - 9 },
                { cx - 1,  cy + 9 }, { cx - 6, cy + 4 }, { cx - 10, cy + 4 }
            };
            Polygon(dc, cone, 6);
            // Plus sign
            MoveToEx(dc, cx + 3, cy, NULL); LineTo(dc, cx + 9, cy);
            MoveToEx(dc, cx + 6, cy - 3, NULL); LineTo(dc, cx + 6, cy + 4);
            break;
        }
        case 1: { // Mudo (Amber speaker + diagonal slash)
            POINT cone[6] = {
                { cx - 10, cy - 4 }, { cx - 6, cy - 4 }, { cx - 1, cy - 9 },
                { cx - 1,  cy + 9 }, { cx - 6, cy + 4 }, { cx - 10, cy + 4 }
            };
            Polygon(dc, cone, 6);
            // Slash line
            HPEN slash_pen = CreatePen(PS_SOLID, 2, COLOR_MUTE_AMBER);
            SelectObject(dc, slash_pen);
            MoveToEx(dc, cx - 9, cy + 9, NULL); LineTo(dc, cx + 9, cy - 9);
            DeleteObject(slash_pen);
            break;
        }
        case 2: { // Play / Pause
            POINT play[3] = { { cx - 11, cy - 9 }, { cx - 2, cy }, { cx - 11, cy + 9 } };
            Polygon(dc, play, 3);
            RECT p1 = { cx + 2, cy - 9, cx + 5, cy + 10 };
            RECT p2 = { cx + 7, cy - 9, cx + 10, cy + 10 };
            FillRect(dc, &p1, brush);
            FillRect(dc, &p2, brush);
            break;
        }
        case 3: { // Anterior (|◀)
            RECT bar = { cx - 10, cy - 9, cx - 8, cy + 10 };
            FillRect(dc, &bar, brush);
            POINT tri[3] = { { cx - 6, cy }, { cx + 7, cy - 9 }, { cx + 7, cy + 9 } };
            Polygon(dc, tri, 3);
            break;
        }
        case 4: { // Próximo (▶|)
            POINT tri[3] = { { cx - 7, cy - 9 }, { cx + 6, cy }, { cx - 7, cy + 9 } };
            Polygon(dc, tri, 3);
            RECT bar = { cx + 8, cy - 9, cx + 10, cy + 10 };
            FillRect(dc, &bar, brush);
            break;
        }
        case 5: { // Volume -
            POINT cone[6] = {
                { cx - 10, cy - 4 }, { cx - 6, cy - 4 }, { cx - 1, cy - 9 },
                { cx - 1,  cy + 9 }, { cx - 6, cy + 4 }, { cx - 10, cy + 4 }
            };
            Polygon(dc, cone, 6);
            // Minus sign
            MoveToEx(dc, cx + 3, cy, NULL); LineTo(dc, cx + 9, cy);
            break;
        }
        case 6: { // Bloco de Notas (Notebook / Spiral pad)
            HBRUSH paper_br = CreateSolidBrush(RGB(240, 244, 250));
            SelectObject(dc, paper_br);
            RoundRect(dc, cx - 11, cy - 13, cx + 11, cy + 13, 4, 4);
            DeleteObject(paper_br);

            // Blue spiral header
            HBRUSH blue_br = CreateSolidBrush(RGB(74, 144, 226));
            RECT hdr = { cx - 11, cy - 13, cx + 11, cy - 7 };
            FillRect(dc, &hdr, blue_br);
            DeleteObject(blue_br);

            // Notebook rule lines
            HPEN line_pen = CreatePen(PS_SOLID, 1, RGB(180, 192, 208));
            SelectObject(dc, line_pen);
            MoveToEx(dc, cx - 7, cy - 2, NULL); LineTo(dc, cx + 7, cy - 2);
            MoveToEx(dc, cx - 7, cy + 3, NULL); LineTo(dc, cx + 7, cy + 3);
            MoveToEx(dc, cx - 7, cy + 8, NULL); LineTo(dc, cx + 4, cy + 8);
            DeleteObject(line_pen);
            break;
        }
        case 7: { // Calculadora
            HBRUSH calc_br = CreateSolidBrush(RGB(50, 58, 72));
            HPEN calc_pen = CreatePen(PS_SOLID, 1, RGB(75, 88, 108));
            SelectObject(dc, calc_br);
            SelectObject(dc, calc_pen);
            RoundRect(dc, cx - 10, cy - 13, cx + 10, cy + 13, 6, 6);
            DeleteObject(calc_br);
            DeleteObject(calc_pen);

            // LCD Display screen
            HBRUSH lcd_br = CreateSolidBrush(RGB(145, 168, 188));
            RECT lcd = { cx - 7, cy - 10, cx + 7, cy - 5 };
            FillRect(dc, &lcd, lcd_br);
            DeleteObject(lcd_br);

            // 6 Button dots
            HBRUSH btn_br = CreateSolidBrush(COLOR_WHITE);
            SelectObject(dc, btn_br);
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 3; ++c) {
                    RECT b = { cx - 6 + c * 5, cy - 1 + r * 5, cx - 3 + c * 5, cy + 2 + r * 5 };
                    FillRect(dc, &b, btn_br);
                }
            }
            DeleteObject(btn_br);
            break;
        }
        case 8: { // Paint (Artist Palette & Brush)
            HBRUSH pal_br = CreateSolidBrush(RGB(244, 246, 250));
            SelectObject(dc, pal_br);
            Ellipse(dc, cx - 13, cy - 9, cx + 13, cy + 9);
            DeleteObject(pal_br);

            // Thumb hole
            HBRUSH hole_br = CreateSolidBrush(RGB(25, 28, 35));
            SelectObject(dc, hole_br);
            Ellipse(dc, cx + 4, cy + 1, cx + 8, cy + 5);
            DeleteObject(hole_br);

            // Paint spots (Red, Yellow, Lime, Blue)
            HBRUSH r_br = CreateSolidBrush(RGB(245, 60, 60));
            SelectObject(dc, r_br); Ellipse(dc, cx - 8, cy - 5, cx - 4, cy - 1); DeleteObject(r_br);

            HBRUSH y_br = CreateSolidBrush(RGB(255, 205, 30));
            SelectObject(dc, y_br); Ellipse(dc, cx - 3, cy - 8, cx + 1, cy - 4); DeleteObject(y_br);

            HBRUSH g_br = CreateSolidBrush(COLOR_ACCENT_LIME);
            SelectObject(dc, g_br); Ellipse(dc, cx + 3, cy - 7, cx + 7, cy - 3); DeleteObject(g_br);

            HBRUSH b_br = CreateSolidBrush(RGB(40, 140, 255));
            SelectObject(dc, b_br); Ellipse(dc, cx - 7, cy + 2, cx - 3, cy + 6); DeleteObject(b_br);

            // Paintbrush
            HPEN brush_pen = CreatePen(PS_SOLID, 2, RGB(195, 140, 85));
            SelectObject(dc, brush_pen);
            MoveToEx(dc, cx + 9, cy - 12, NULL); LineTo(dc, cx - 2, cy + 6);
            DeleteObject(brush_pen);
            break;
        }
        case 9: { // Pasta (Folder)
            HBRUSH f_br = CreateSolidBrush(RGB(240, 244, 250));
            SelectObject(dc, f_br);
            POINT tab[4] = { { cx - 11, cy - 9 }, { cx - 4, cy - 9 }, { cx - 2, cy - 6 }, { cx - 11, cy - 6 } };
            Polygon(dc, tab, 4);
            RoundRect(dc, cx - 12, cy - 6, cx + 12, cy + 8, 4, 4);
            DeleteObject(f_br);
            break;
        }
        case 10: { // Atalho (Keyboard)
            HBRUSH kb_br = CreateSolidBrush(RGB(45, 52, 65));
            HPEN kb_pen = CreatePen(PS_SOLID, 1, RGB(90, 105, 125));
            SelectObject(dc, kb_br);
            SelectObject(dc, kb_pen);
            RoundRect(dc, cx - 13, cy - 7, cx + 13, cy + 8, 4, 4);
            DeleteObject(kb_br);
            DeleteObject(kb_pen);

            // Miniature key dots
            HBRUSH key_br = CreateSolidBrush(COLOR_WHITE);
            SelectObject(dc, key_br);
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 5; ++c) {
                    RECT k = { cx - 10 + c * 4, cy - 4 + r * 4, cx - 8 + c * 4, cy - 2 + r * 4 };
                    FillRect(dc, &k, key_br);
                }
            }
            // Spacebar
            RECT sp = { cx - 6, cy + 4, cx + 6, cy + 6 };
            FillRect(dc, &sp, key_br);
            DeleteObject(key_br);
            break;
        }
        default: { // Empty slot centered +
            HPEN plus_pen = CreatePen(PS_SOLID, 1, RGB(65, 72, 85));
            SelectObject(dc, plus_pen);
            MoveToEx(dc, cx - 4, cy, NULL); LineTo(dc, cx + 5, cy);
            MoveToEx(dc, cx, cy - 4, NULL); LineTo(dc, cx, cy + 5);
            DeleteObject(plus_pen);
            break;
        }
    }

    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(pen);
    DeleteObject(brush);
}

static const wchar_t *get_key_default_title(int idx) {
    switch (idx) {
        case 0: return L"Volume +";
        case 1: return L"Mudo";
        case 2: return L"Play / Pause";
        case 3: return L"Anterior";
        case 4: return L"Próximo";
        case 5: return L"Volume -";
        case 6: return L"Bloco de Notas";
        case 7: return L"Calculadora";
        case 8: return L"Paint";
        case 9: return L"Pasta";
        case 10: return L"Atalho";
        default: return L"";
    }
}

// Draw Individual 15 Keys
static void draw_key(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    int idx = draw->CtlID - ID_KEY_BASE;
    BOOL is_selected = (idx == g_selected_key);
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    SetBkMode(dc, TRANSPARENT);

    // Key base
    COLORREF bg_col = is_pressed ? COLOR_KEY_PRESSED : (is_selected ? RGB(20, 24, 28) : COLOR_KEY_BASE);
    HBRUSH brush = CreateSolidBrush(bg_col);
    HPEN pen = CreatePen(PS_SOLID, 1, is_selected ? COLOR_ACCENT_LIME : COLOR_BORDER_KEY);
    HGDIOBJ old_b = SelectObject(dc, brush);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 14, 14);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(brush);
    DeleteObject(pen);

    // Glowing border for selected key
    if (is_selected) {
        draw_lime_glow_border(dc, rect, 14);
    }

    RECT content = rect;
    if (is_pressed) OffsetRect(&content, 0, 1);

    int cx = (content.left + content.right) / 2;
    int cy = content.top + (content.bottom - content.top) / 2 - 10;

    // Custom Icon or Vector Icon
    HICON app_icon = NULL;
    BOOL owns_icon = FALSE;
    if (idx >= 6 && idx <= 8) {
        int app_idx = idx - 6;
        const DeckApp *app = &g_config.apps[app_idx];
        if (app->icon_path[0]) {
            app_icon = (HICON)LoadImageA(NULL, app->icon_path, IMAGE_ICON, 36, 36, LR_LOADFROMFILE);
            owns_icon = (app_icon != NULL);
        }
    }

    if (app_icon) {
        int sz = 36;
        DrawIconEx(dc, cx - sz / 2, cy - sz / 2, app_icon, sz, sz, 0, NULL, DI_NORMAL);
        if (owns_icon) DestroyIcon(app_icon);
    } else {
        draw_key_icon(dc, idx, cx, cy, (idx == 1) ? COLOR_MUTE_AMBER : COLOR_WHITE);
    }

    // Title label
    const wchar_t *title = NULL;
    static wchar_t custom_name[DECK_TEXT_MAX];

    if (idx >= 6 && idx <= 8) {
        int app_idx = idx - 6;
        if (g_config.apps[app_idx].configured) {
            utf8_to_wide(g_config.apps[app_idx].name, custom_name, DECK_TEXT_MAX);
            title = custom_name;
        } else {
            if (idx == 6) title = L"Bloco de Notas";
            else if (idx == 7) title = L"Calculadora";
            else title = L"Paint";
        }
    } else {
        title = get_key_default_title(idx);
    }

    if (title && title[0]) {
        HGDIOBJ old_f = SelectObject(dc, g_font_label);
        SetTextColor(dc, is_selected ? COLOR_WHITE : COLOR_TEXT_MUTED);
        RECT tr = { content.left + 4, content.bottom - 26, content.right - 4, content.bottom - 6 };
        DrawTextW(dc, title, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old_f);
    }
}

// Sidebar Category Buttons
static void draw_sidebar_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    int id = draw->CtlID;
    BOOL is_active = (id == g_selected_category);

    SetBkMode(dc, TRANSPARENT);

    if (is_active) {
        HBRUSH br = CreateSolidBrush(COLOR_ACCENT_BG);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(38, 65, 30));
        HGDIOBJ old_b = SelectObject(dc, br);
        HGDIOBJ old_p = SelectObject(dc, pen);
        RoundRect(dc, rect.left + 4, rect.top + 1, rect.right - 4, rect.bottom - 1, 10, 10);
        SelectObject(dc, old_b);
        SelectObject(dc, old_p);
        DeleteObject(br);
        DeleteObject(pen);

        // Vertical lime indicator on the far left
        RECT bar = { rect.left + 6, rect.top + 6, rect.left + 10, rect.bottom - 6 };
        HBRUSH acc = CreateSolidBrush(COLOR_ACCENT_LIME);
        FillRect(dc, &bar, acc);
        DeleteObject(acc);
    } else {
        HBRUSH br = CreateSolidBrush(COLOR_SIDEBAR);
        FillRect(dc, &rect, br);
        DeleteObject(br);
    }

    COLORREF icon_col = is_active ? COLOR_ACCENT_LIME : COLOR_TEXT_MUTED;
    draw_category_icon(dc, id, rect.left + 24, (rect.top + rect.bottom) / 2, icon_col);

    const wchar_t *label = L"";
    switch (id) {
        case ID_LIB_FAVORITES:   label = L"Favoritos"; break;
        case ID_LIB_SYSTEM:      label = L"Sistema"; break;
        case ID_LIB_AUDIO:       label = L"Áudio"; break;
        case ID_LIB_MEDIA:       label = L"Mídia"; break;
        case ID_LIB_APPS:        label = L"Aplicativos"; break;
        case ID_LIB_NAVIGATION:  label = L"Navegação"; break;
        case ID_LIB_MULTIACTION: label = L"Multi Action"; break;
    }

    HGDIOBJ old_f = SelectObject(dc, is_active ? g_font_btn : g_font_label);
    SetTextColor(dc, is_active ? COLOR_WHITE : COLOR_TEXT_MUTED);
    RECT tr = { rect.left + 42, rect.top, rect.right - 8, rect.bottom };
    DrawTextW(dc, label, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_f);
}

// Top Bar Profile Button
static void draw_profile_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    COLORREF bg = is_pressed ? RGB(26, 29, 36) : RGB(34, 38, 47);
    HBRUSH br = CreateSolidBrush(bg);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(54, 61, 74));
    HGDIOBJ old_b = SelectObject(dc, br);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(br);
    DeleteObject(pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COLOR_WHITE);
    HGDIOBJ old_f = SelectObject(dc, g_font_label);
    RECT tr = { rect.left + 14, rect.top, rect.right - 24, rect.bottom };
    DrawTextW(dc, L"Default Profile", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Chevron down
    HPEN chev_pen = CreatePen(PS_SOLID, 2, RGB(160, 172, 190));
    SelectObject(dc, chev_pen);
    int cx = rect.right - 18;
    int cy = (rect.top + rect.bottom) / 2;
    MoveToEx(dc, cx - 4, cy - 2, NULL); LineTo(dc, cx, cy + 2);
    LineTo(dc, cx + 4, cy - 2);
    SelectObject(dc, old_p);
    DeleteObject(chev_pen);
    SelectObject(dc, old_f);
}

// Top Bar Settings Button
static void draw_settings_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    COLORREF bg = is_pressed ? RGB(26, 29, 36) : RGB(34, 38, 47);
    HBRUSH br = CreateSolidBrush(bg);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(54, 61, 74));
    HGDIOBJ old_b = SelectObject(dc, br);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(br);
    DeleteObject(pen);

    int gx = rect.left + 16;
    int gy = (rect.top + rect.bottom) / 2;
    draw_category_icon(dc, ID_LIB_SYSTEM, gx, gy, RGB(220, 226, 235));

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COLOR_WHITE);
    HGDIOBJ old_f = SelectObject(dc, g_font_label);
    RECT tr = { rect.left + 30, rect.top, rect.right - 8, rect.bottom };
    DrawTextW(dc, L"Settings", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_f);
}

// Inspector Action Buttons (Executar Ação, Salvar alterações, Remover Ação)
static void draw_inspector_button(const DRAWITEMSTRUCT *draw, const wchar_t *text) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    COLORREF bg = is_pressed ? RGB(26, 29, 36) : RGB(36, 40, 50);
    COLORREF border = is_pressed ? COLOR_ACCENT_LIME : COLOR_BORDER_BTN;

    HBRUSH br = CreateSolidBrush(bg);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_b = SelectObject(dc, br);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(br);
    DeleteObject(pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, COLOR_WHITE);
    HGDIOBJ old_f = SelectObject(dc, g_font_btn);
    RECT tr = rect;
    if (is_pressed) OffsetRect(&tr, 0, 1);
    DrawTextW(dc, text, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_f);
}

static void draw_browse_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    COLORREF bg = is_pressed ? RGB(26, 29, 36) : RGB(36, 40, 50);
    HBRUSH br = CreateSolidBrush(bg);
    HPEN pen = CreatePen(PS_SOLID, 1, COLOR_BORDER_BTN);
    HGDIOBJ old_b = SelectObject(dc, br);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 8, 8);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(br);
    DeleteObject(pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(220, 225, 235));
    HGDIOBJ old_f = SelectObject(dc, g_font_btn);
    DrawTextW(dc, L"...", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_f);
}

// Property Inspector Preview Box (Bottom Right)
static void draw_inspector_preview(HDC dc, RECT rect) {
    int cx = (rect.left + rect.right) / 2;
    int cy = (rect.top + rect.bottom) / 2;

    // Ambient radial glow behind preview box in the bottom right corner
    HBRUSH glow1 = CreateSolidBrush(RGB(22, 38, 18));
    HPEN null_pen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ old_b = SelectObject(dc, glow1);
    HGDIOBJ old_p = SelectObject(dc, null_pen);
    Ellipse(dc, cx - 48, cy - 48, cx + 48, cy + 48);

    HBRUSH glow2 = CreateSolidBrush(RGB(28, 54, 22));
    SelectObject(dc, glow2);
    Ellipse(dc, cx - 36, cy - 36, cx + 36, cy + 36);
    DeleteObject(glow1);
    DeleteObject(glow2);

    // Preview key box
    HBRUSH key_br = CreateSolidBrush(COLOR_KEY_BASE);
    SelectObject(dc, key_br);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 14, 14);
    DeleteObject(key_br);

    // Glowing lime-green border
    draw_lime_glow_border(dc, rect, 14);

    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(null_pen);

    // Render key icon inside preview
    draw_key_icon(dc, g_selected_key, cx, rect.top + 26, (g_selected_key == 1) ? COLOR_MUTE_AMBER : COLOR_WHITE);

    // Label below icon
    const wchar_t *title = L"";
    static wchar_t wbuf[DECK_TEXT_MAX];
    if (g_selected_key >= 6 && g_selected_key <= 8) {
        int app_idx = g_selected_key - 6;
        if (g_config.apps[app_idx].configured) {
            utf8_to_wide(g_config.apps[app_idx].name, wbuf, DECK_TEXT_MAX);
            title = wbuf;
        } else {
            if (g_selected_key == 6) title = L"Bloco de Notas";
            else if (g_selected_key == 7) title = L"Calculadora";
            else title = L"Paint";
        }
    } else {
        title = get_key_default_title(g_selected_key);
    }

    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ old_f = SelectObject(dc, g_font_sub);
    SetTextColor(dc, COLOR_WHITE);
    RECT tr = { rect.left + 4, rect.bottom - 22, rect.right - 4, rect.bottom - 4 };
    DrawTextW(dc, title, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, old_f);
}

// Window Procedure
static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (g_wm_close_msg != 0 && message == g_wm_close_msg) {
        remove_tray_icon();
        DestroyWindow(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    switch (message) {
        case WM_CREATE: {
            HINSTANCE inst = ((LPCREATESTRUCTW)lparam)->hInstance;
            g_wm_close_msg = RegisterWindowMessageW(L"CDeck_CloseMessage");

            // Top Bar Profile Selector Button
            CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                172, 10, 150, 32, hwnd, (HMENU)ID_PROFILE_BTN, inst, NULL);

            // Top Bar Settings Button
            CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 104, 32, hwnd, (HMENU)ID_SETTINGS, inst, NULL);

            // Action Library Sidebar Buttons
            for (int i = 0; i < 7; ++i) {
                CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    0, 0, 100, 36, hwnd, (HMENU)(INT_PTR)(ID_LIB_FAVORITES + i), inst, NULL);
            }

            // 15 Keys in Canvas
            for (int i = 0; i < ID_KEY_COUNT; ++i) {
                g_keys[i] = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    0, 0, 96, 96, hwnd, (HMENU)(INT_PTR)(ID_KEY_BASE + i), inst, NULL);
            }

            // Property Inspector Controls
            g_insp_title_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_TITLE, inst, NULL);
            SendMessageW(g_insp_title_edit, WM_SETFONT, (WPARAM)g_font_btn, TRUE);

            g_insp_path_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_PATH, inst, NULL);
            SendMessageW(g_insp_path_edit, WM_SETFONT, (WPARAM)g_font_btn, TRUE);

            g_insp_path_browse = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 32, 28, hwnd, (HMENU)ID_INSP_BROWSE_PATH, inst, NULL);

            g_insp_test_btn = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_TEST, inst, NULL);

            g_insp_save_btn = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_SAVE, inst, NULL);

            g_insp_remove_btn = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_REMOVE, inst, NULL);

            // Audio inspector controls
            g_insp_vol_track = CreateWindowExW(0, L"msctls_trackbar32", L"",
                WS_CHILD | TBS_NOTICKS | TBS_HORZ,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_VOL_TRACK, inst, NULL);
            if (g_insp_vol_track) {
                SendMessageW(g_insp_vol_track, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            }
            g_insp_mute_btn = CreateWindowW(L"BUTTON", L"Alternar Mudo", WS_CHILD | BS_PUSHBUTTON,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_MUTE_BTN, inst, NULL);

            // Bottom status label
            g_status = CreateWindowW(L"STATIC", L"Pronto. C-Deck Stream Deck Edition.",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                16, 0, 400, 20, hwnd, (HMENU)ID_STATUS, inst, NULL);
            if (g_status && g_font_sub) SendMessageW(g_status, WM_SETFONT, (WPARAM)g_font_sub, TRUE);

            update_inspector_ui();
            layout_controls(hwnd);
            SetTimer(hwnd, ID_VOLUME_TIMER, 500, NULL);

            RegisterHotKey(hwnd, ID_HOTKEY_UP, MOD_CONTROL | MOD_ALT, VK_UP);
            RegisterHotKey(hwnd, ID_HOTKEY_DOWN, MOD_CONTROL | MOD_ALT, VK_DOWN);
            RegisterHotKey(hwnd, ID_HOTKEY_MUTE, MOD_CONTROL | MOD_ALT, 'M');
            return 0;
        }

        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) {
                if (add_tray_icon(hwnd)) ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            layout_controls(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = 980;
            limits->ptMinTrackSize.y = 640;
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client;
            GetClientRect(hwnd, &client);
            int width = client.right - client.left;
            int height = client.bottom - client.top;
            int topbar_h = 52;
            int statusbar_h = 28;
            int sidebar_w = 200;
            int inspector_w = 320;
            int canvas_x = sidebar_w;
            int canvas_w = width - sidebar_w - inspector_w;

            SetBkMode(dc, TRANSPARENT);

            // 1. Top Bar
            RECT top_rect = { 0, 0, width, topbar_h };
            HBRUSH top_br = CreateSolidBrush(COLOR_TOPBAR);
            FillRect(dc, &top_rect, top_br);
            DeleteObject(top_br);

            HPEN div_pen = CreatePen(PS_SOLID, 1, COLOR_BORDER_DARK);
            HGDIOBJ old_pen = SelectObject(dc, div_pen);
            MoveToEx(dc, 0, topbar_h - 1, NULL); LineTo(dc, width, topbar_h - 1);

            // Draw Official C-Deck Logo mark & text
            draw_logo(dc, 22, 14);

            HGDIOBJ old_f = SelectObject(dc, g_font_title);
            SetTextColor(dc, COLOR_WHITE);
            RECT logo_rect = { 56, 0, 160, topbar_h };
            DrawTextW(dc, L"C-DECK", -1, &logo_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // 2. Sidebar Background & Border
            RECT side_rect = { 0, topbar_h, sidebar_w, height - statusbar_h };
            HBRUSH side_br = CreateSolidBrush(COLOR_SIDEBAR);
            FillRect(dc, &side_rect, side_br);
            DeleteObject(side_br);

            MoveToEx(dc, sidebar_w - 1, topbar_h, NULL); LineTo(dc, sidebar_w - 1, height - statusbar_h);

            // 3. Central Canvas
            RECT can_rect = { canvas_x, topbar_h, canvas_x + canvas_w, height - statusbar_h };
            FillRect(dc, &can_rect, g_brush_bg);

            // Header: LAYOUT DO DISPOSITIVO / STREAM DECK - 15 TECLAS
            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_DIM);
            RECT dev_lbl = { canvas_x + 36, topbar_h + 16, canvas_x + canvas_w, topbar_h + 30 };
            DrawTextW(dc, L"LAYOUT DO DISPOSITIVO", -1, &dev_lbl, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(dc, g_font_header);
            SetTextColor(dc, COLOR_WHITE);
            RECT dev_name = { canvas_x + 36, topbar_h + 30, canvas_x + canvas_w, topbar_h + 52 };
            DrawTextW(dc, L"STREAM DECK - 15 TECLAS", -1, &dev_name, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // 4. Property Inspector (Right Panel)
            int insp_x = width - inspector_w;
            RECT insp_rect = { insp_x, topbar_h, width, height - statusbar_h };
            HBRUSH insp_br = CreateSolidBrush(COLOR_INSPECTOR);
            FillRect(dc, &insp_rect, insp_br);
            DeleteObject(insp_br);

            MoveToEx(dc, insp_x, topbar_h, NULL); LineTo(dc, insp_x, height - statusbar_h);

            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_DIM);
            RECT pr_hdr = { insp_x + 22, topbar_h + 16, width - 20, topbar_h + 30 };
            DrawTextW(dc, L"PROPRIEDADES:", -1, &pr_hdr, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(dc, g_font_header);
            SetTextColor(dc, COLOR_WHITE);
            RECT pr_type = { insp_x + 22, topbar_h + 30, width - 20, topbar_h + 52 };
            const wchar_t *act_hdr = (g_selected_key >= 6 && g_selected_key <= 8) ? L"Sistema: Atalho"
                : ((g_selected_key <= 1 || g_selected_key == 5) ? L"Sistema: Controle de Áudio"
                : ((g_selected_key == 2) ? L"Sistema: Mute" : L"Sistema: Multimídia"));
            DrawTextW(dc, act_hdr, -1, &pr_type, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Labels inside Property Inspector for configured app slots
            if (g_selected_key >= 6 && g_selected_key <= 8) {
                SelectObject(dc, g_font_label);
                SetTextColor(dc, COLOR_TEXT_MUTED);
                RECT l1 = { insp_x + 22, topbar_h + 62, width - 22, topbar_h + 78 };
                DrawTextW(dc, L"Título", -1, &l1, DT_LEFT | DT_SINGLELINE);
                RECT l2 = { insp_x + 22, topbar_h + 116, width - 22, topbar_h + 132 };
                DrawTextW(dc, L"Aplicativo / arquivo / URL", -1, &l2, DT_LEFT | DT_SINGLELINE);

                // Preview section at bottom
                RECT prev_hdr = { insp_x + 22, height - statusbar_h - 118, width - 22, height - statusbar_h - 102 };
                DrawTextW(dc, L"Prévia da tecla", -1, &prev_hdr, DT_LEFT | DT_SINGLELINE);
                RECT prev_box = { insp_x + 22, height - statusbar_h - 96, insp_x + 94, height - statusbar_h - 24 };
                draw_inspector_preview(dc, prev_box);
            }

            // 5. Bottom Status Area
            RECT stat_rect = { 0, height - statusbar_h, width, height };
            HBRUSH stat_br = CreateSolidBrush(COLOR_BG);
            FillRect(dc, &stat_rect, stat_br);
            DeleteObject(stat_br);

            MoveToEx(dc, 0, height - statusbar_h, NULL); LineTo(dc, width, height - statusbar_h);

            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_DIM);
            RECT info_rect = { width - 240, height - statusbar_h, width - 16, height };
            DrawTextW(dc, L"Windows Core Audio · C-Deck Native", -1, &info_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(dc, old_pen);
            DeleteObject(div_pen);
            SelectObject(dc, old_f);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DRAWITEM: {
            const DRAWITEMSTRUCT *draw = (const DRAWITEMSTRUCT *)lparam;
            if (draw->CtlID == ID_PROFILE_BTN) {
                draw_profile_button(draw);
                return TRUE;
            }
            if (draw->CtlID == ID_SETTINGS) {
                draw_settings_button(draw);
                return TRUE;
            }
            if (draw->CtlID >= ID_KEY_BASE && draw->CtlID < ID_KEY_BASE + ID_KEY_COUNT) {
                draw_key(draw);
                return TRUE;
            }
            if (draw->CtlID >= ID_LIB_FAVORITES && draw->CtlID <= ID_LIB_MULTIACTION) {
                draw_sidebar_button(draw);
                return TRUE;
            }
            if (draw->CtlID == ID_INSP_TEST) {
                draw_inspector_button(draw, L"Executar Ação");
                return TRUE;
            }
            if (draw->CtlID == ID_INSP_SAVE) {
                draw_inspector_button(draw, L"Salvar alterações");
                return TRUE;
            }
            if (draw->CtlID == ID_INSP_REMOVE) {
                draw_inspector_button(draw, L"Remover Ação");
                return TRUE;
            }
            if (draw->CtlID == ID_INSP_BROWSE_PATH) {
                draw_browse_button(draw);
                return TRUE;
            }
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, COLOR_WHITE);
            SetBkColor(dc, COLOR_INPUT_BG);
            return (LRESULT)g_brush_input;
        }

        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, COLOR_TEXT_MUTED);
            SetBkColor(dc, COLOR_BG);
            return (LRESULT)g_brush_bg;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);

            if (id >= ID_KEY_BASE && id < ID_KEY_BASE + ID_KEY_COUNT) {
                int clicked = id - ID_KEY_BASE;
                g_selected_key = clicked;
                update_inspector_ui();
                execute_key_action(clicked);
                return 0;
            }

            if (id >= ID_LIB_FAVORITES && id <= ID_LIB_MULTIACTION) {
                g_selected_category = id;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (id == ID_INSP_TEST) {
                execute_key_action(g_selected_key);
                return 0;
            }

            if (id == ID_INSP_SAVE) {
                save_inspector_changes();
                return 0;
            }

            if (id == ID_INSP_REMOVE) {
                remove_selected_action();
                return 0;
            }

            if (id == ID_INSP_MUTE_BTN) {
                action_toggle_mute();
                update_inspector_ui();
                return 0;
            }

            if (id == ID_INSP_BROWSE_PATH) {
                browse_file_for_inspector(hwnd);
                return 0;
            }

            if (id == ID_SETTINGS || id == ID_PROFILE_BTN) {
                set_status(L"Configuração de perfis ativa. Perfil atual: Default Profile.");
                return 0;
            }

            if (id == ID_TRAY_OPEN) {
                remove_tray_icon();
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                return 0;
            }

            if (id == ID_TRAY_EXIT) {
                remove_tray_icon();
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                return 0;
            }
            break;
        }

        case WM_HSCROLL:
            if ((HWND)lparam == g_insp_vol_track) {
                int pos = (int)SendMessageW(g_insp_vol_track, TBM_GETPOS, 0, 0);
                action_set_volume((float)pos / 100.0f);
                wchar_t msg[64];
                swprintf(msg, 64, L"Volume ajustado: %d%%.", pos);
                set_status(msg);
            }
            return 0;

        case WM_TIMER:
            if (wparam == ID_VOLUME_TIMER && GetCapture() != g_insp_vol_track) {
                float level = 0.0f; int muted = 0;
                if (action_get_volume(&level, &muted) == ACTION_SUCCESS && g_insp_vol_track) {
                    SendMessageW(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
                }
            }
            return 0;

        case WM_HOTKEY:
            if (wparam == ID_HOTKEY_UP) report_result(action_volume_step(0.05f), L"Volume aumentado (+5%).");
            else if (wparam == ID_HOTKEY_DOWN) report_result(action_volume_step(-0.05f), L"Volume reduzido (−5%).");
            else if (wparam == ID_HOTKEY_MUTE) report_result(action_toggle_mute(), L"Mudo alternado.");
            update_inspector_ui();
            return 0;

        case WM_TRAYICON:
            if (lparam == WM_LBUTTONDBLCLK) {
                remove_tray_icon();
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            } else if (lparam == WM_RBUTTONUP) {
                show_tray_menu(hwnd);
            }
            return 0;

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_ENDSESSION:
            if (wparam) {
                remove_tray_icon();
                DestroyWindow(hwnd);
                PostQuitMessage(0);
            }
            return 0;

        case WM_CLOSE:
            if (add_tray_icon(hwnd)) {
                ShowWindow(hwnd, SW_HIDE);
            } else {
                set_status(L"Não foi possível minimizar para a bandeja.");
                MessageBoxW(hwnd, L"O Windows não permitiu criar o ícone da bandeja. O C-Deck continuará aberto.", L"C-Deck", MB_OK | MB_ICONWARNING);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_VOLUME_TIMER);
            UnregisterHotKey(hwnd, ID_HOTKEY_UP);
            UnregisterHotKey(hwnd, ID_HOTKEY_DOWN);
            UnregisterHotKey(hwnd, ID_HOTKEY_MUTE);
            remove_tray_icon();

            if (g_font_title) DeleteObject(g_font_title);
            if (g_font_header) DeleteObject(g_font_header);
            if (g_font_label) DeleteObject(g_font_label);
            if (g_font_sub) DeleteObject(g_font_sub);
            if (g_font_btn) DeleteObject(g_font_btn);

            if (g_brush_bg) DeleteObject(g_brush_bg);
            if (g_brush_panel) DeleteObject(g_brush_panel);
            if (g_brush_input) DeleteObject(g_brush_input);

            g_font_title = NULL;
            g_font_header = NULL;
            g_font_label = NULL;
            g_font_sub = NULL;
            g_font_btn = NULL;

            g_brush_bg = NULL;
            g_brush_panel = NULL;
            g_brush_input = NULL;

            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

int interface_run(HINSTANCE instance, int show_command, const DeckConfig *config) {
    WNDCLASSEXW window_class;
    HWND hwnd;
    MSG message;

    if (!config || !build_config_path(g_config_path, MAX_PATH)) return 1;
    g_config = *config;

    {
        INITCOMMONCONTROLSEX controls;
        controls.dwSize = sizeof(controls);
        controls.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&controls);
    }

    // Typography (Segoe UI ClearType Quality)
    g_font_title = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_font_header = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_font_label = CreateFontW(12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_font_sub = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_font_btn = CreateFontW(12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Brushes
    g_brush_bg = CreateSolidBrush(COLOR_BG);
    g_brush_panel = CreateSolidBrush(COLOR_SIDEBAR);
    g_brush_input = CreateSolidBrush(COLOR_INPUT_BG);

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_APP_ICON));
    window_class.hIconSm = window_class.hIcon;
    window_class.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    window_class.hbrBackground = g_brush_bg;
    window_class.lpszClassName = CLASS_NAME;
    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    // Desktop Dimensions: 1200 x 760
    hwnd = CreateWindowExW(
        0, CLASS_NAME, L"C-DECK | Elgato Stream Deck Edition",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760,
        NULL, NULL, instance, NULL
    );
    if (!hwnd) return 1;
    g_main_window = hwnd;

    enable_dark_titlebar(hwnd);

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    g_main_window = NULL;
    return (int)message.wParam;
}
