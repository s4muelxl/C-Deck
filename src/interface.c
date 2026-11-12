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
#define CLASS_NAME          "CDeckMainWindow"

// Colors matching Elgato Stream Deck visual specification
#define COLOR_BG            RGB(23, 25, 29)    // #17191D (Window canvas background)
#define COLOR_PANEL         RGB(34, 37, 43)    // #22252B (Sidebars and panels)
#define COLOR_PANEL_ELEV    RGB(43, 47, 54)    // #2B2F36 (Inputs, headers, elevated cards)
#define COLOR_KEY           RGB(16, 18, 22)    // #101216 (LCD keycap base)
#define COLOR_KEY_HOVER     RGB(26, 30, 37)    // #1A1E25 (Key hover)
#define COLOR_BORDER        RGB(65, 71, 80)    // #414750 (Subtle key/panel border)
#define COLOR_BORDER_SUBTLE RGB(46, 51, 60)    // #2E333C (Panel separators)
#define COLOR_TEXT          RGB(242, 244, 247) // #F2F4F7 (Primary text)
#define COLOR_TEXT_MUTED    RGB(166, 173, 183) // #A6ADB7 (Secondary labels)
#define COLOR_ACCENT_BLUE   RGB(22, 139, 255)  // #168BFF (Key selection outline & primary buttons)
#define COLOR_ACCENT_HOVER  RGB(45, 155, 255)  // #2D9BFF
#define COLOR_SUCCESS       RGB(45, 190, 117)  // #2DBE75 (Success indicators)
#define COLOR_WARNING       RGB(240, 181, 68)  // #F0B544 (Warning / mute active)
#define COLOR_DANGER        RGB(255, 82, 82)   // #FF5252 (Mute coral / remove)

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
#define ID_INSP_ICON        1204
#define ID_INSP_BROWSE_ICON 1205
#define ID_INSP_SAVE        1206
#define ID_INSP_REMOVE      1207
#define ID_INSP_TEST        1208
#define ID_INSP_VOL_TRACK   1209
#define ID_INSP_MUTE_BTN    1210

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
static int g_selected_key = 8; // Default to key 8 (OBS Studio / App 3)
static int g_selected_category = ID_LIB_SYSTEM;

// Inspector controls
static HWND g_insp_title_edit;
static HWND g_insp_path_edit;
static HWND g_insp_path_browse;
static HWND g_insp_icon_edit;
static HWND g_insp_icon_browse;
static HWND g_insp_save_btn;
static HWND g_insp_remove_btn;
static HWND g_insp_test_btn;
static HWND g_insp_vol_track;
static HWND g_insp_mute_btn;

// Fonts and Brushes
static HFONT g_font_title;
static HFONT g_font_header;
static HFONT g_font_label;
static HFONT g_font_sub;
static HFONT g_font_glyph;
static HFONT g_font_btn;
static HBRUSH g_brush_bg;
static HBRUSH g_brush_panel;
static HBRUSH g_brush_elevated;
static HBRUSH g_brush_key;
static HBRUSH g_brush_accent;

static NOTIFYICONDATAA g_tray_data;
static int g_tray_added;
static UINT g_wm_close_msg;

// Forward declarations
static void layout_controls(HWND hwnd);
static void update_inspector_ui(void);
static void draw_key(const DRAWITEMSTRUCT *draw);
static void draw_sidebar_button(const DRAWITEMSTRUCT *draw);
static void draw_inspector_preview(HDC dc, RECT rect);

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

static void set_status(const char *message) {
    if (g_status) SetWindowTextA(g_status, message);
}

static void report_result(ActionResult result, const char *success_message) {
    switch (result) {
        case ACTION_SUCCESS: set_status(success_message); break;
        case ACTION_AUDIO_UNAVAILABLE: set_status("Dispositivo de audio indisponivel."); break;
        case ACTION_MEDIA_UNAVAILABLE: set_status("O Windows nao aceitou o comando de midia."); break;
        case ACTION_LAUNCH_FAILED: set_status("Nao foi possivel abrir o aplicativo. Confira o caminho."); break;
        case ACTION_NOT_CONFIGURED: set_status("Atalho nao configurado. Preencha os campos ao lado."); break;
        default: set_status("Nao foi possivel executar esta acao."); break;
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
    if (!g_tray_data.hIcon) g_tray_data.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    lstrcpynA(g_tray_data.szTip, "C-Deck — Elgato Stream Deck Edition", (int)sizeof(g_tray_data.szTip));
    g_tray_added = Shell_NotifyIconA(NIM_ADD, &g_tray_data) ? 1 : 0;
    return g_tray_added;
}

static void remove_tray_icon(void) {
    if (g_tray_added) Shell_NotifyIconA(NIM_DELETE, &g_tray_data);
    g_tray_added = 0;
}

static void show_tray_menu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuA(menu, MF_STRING, ID_TRAY_OPEN, "Abrir C-Deck");
    AppendMenuA(menu, MF_STRING, ID_TRAY_EXIT, "Sair");
    SetForegroundWindow(hwnd);
    {
        POINT point;
        UINT command;
        GetCursorPos(&point);
        command = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, 0, hwnd, NULL);
        PostMessageA(hwnd, WM_NULL, 0, 0);
        if (command) SendMessageA(hwnd, WM_COMMAND, command, 0);
    }
    DestroyMenu(menu);
}

// Key Action Execution
static void execute_key_action(int key_index) {
    switch (key_index) {
        case 0: // Volume +
            report_result(action_volume_step(0.05f), "Volume aumentado em +5%.");
            break;
        case 1: // Mudo
            report_result(action_toggle_mute(), "Mudo alternado com sucesso.");
            break;
        case 2: // Play / Pause
            report_result(action_media_play_pause(), "Play / Pause enviado ao Windows.");
            break;
        case 3: // Faixa Anterior
            report_result(action_media_previous(), "Comando de faixa anterior enviado.");
            break;
        case 4: // Proxima Faixa
            report_result(action_media_next(), "Comando de proxima faixa enviado.");
            break;
        case 5: // Volume -
            report_result(action_volume_step(-0.05f), "Volume reduzido em −5%.");
            break;
        case 6: // App Slot 1
            report_result(action_launch_app(&g_config.apps[0]), "Atalho executado.");
            break;
        case 7: // App Slot 2
            report_result(action_launch_app(&g_config.apps[1]), "Atalho executado.");
            break;
        case 8: // App Slot 3
            report_result(action_launch_app(&g_config.apps[2]), "Atalho executado.");
            break;
        case 9: // Pasta
            ShellExecuteA(NULL, "open", "explorer.exe", NULL, NULL, SW_SHOWNORMAL);
            set_status("Abrindo Explorador de Arquivos.");
            break;
        default:
            set_status("Slot vazio selecionado. Configure ao lado.");
            break;
    }
    update_inspector_ui();
}

static void browse_file_for_inspector(HWND hwnd, int choose_icon) {
    OPENFILENAMEA dialog;
    char selected[DECK_TEXT_MAX];
    const char *filter = choose_icon
        ? "Icones do Windows (*.ico)\0*.ico\0Todos os arquivos (*.*)\0*.*\0\0"
        : "Aplicativos (*.exe;*.com;*.bat;*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Todos os arquivos (*.*)\0*.*\0\0";
    HWND target_edit = choose_icon ? g_insp_icon_edit : g_insp_path_edit;
    ZeroMemory(&dialog, sizeof(dialog));
    ZeroMemory(selected, sizeof(selected));
    if (target_edit) GetWindowTextA(target_edit, selected, sizeof(selected));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = selected;
    dialog.nMaxFile = sizeof(selected);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = choose_icon ? "Escolher icone (.ico)" : "Escolher aplicativo (.exe)";
    if (GetOpenFileNameA(&dialog)) {
        if (target_edit) SetWindowTextA(target_edit, selected);
    }
}

static void save_inspector_changes(void) {
    if (g_selected_key < 6 || g_selected_key > 8) return;
    int app_idx = g_selected_key - 6;
    char name[DECK_TEXT_MAX];
    char path[DECK_TEXT_MAX];
    char icon[DECK_TEXT_MAX];

    GetWindowTextA(g_insp_title_edit, name, sizeof(name));
    GetWindowTextA(g_insp_path_edit, path, sizeof(path));
    GetWindowTextA(g_insp_icon_edit, icon, sizeof(icon));

    // Trim whitespace
    char *s = name; while (*s && isspace((unsigned char)*s)) ++s; memmove(name, s, strlen(s) + 1);
    char *e = name + strlen(name); while (e > name && isspace((unsigned char)e[-1])) *--e = '\0';

    s = path; while (*s && isspace((unsigned char)*s)) ++s; memmove(path, s, strlen(s) + 1);
    e = path + strlen(path); while (e > path && isspace((unsigned char)e[-1])) *--e = '\0';

    s = icon; while (*s && isspace((unsigned char)*s)) ++s; memmove(icon, s, strlen(s) + 1);
    e = icon + strlen(icon); while (e > icon && isspace((unsigned char)e[-1])) *--e = '\0';

    strcpy(g_config.apps[app_idx].name, name);
    strcpy(g_config.apps[app_idx].path, path);
    strcpy(g_config.apps[app_idx].icon_path, (name[0] && path[0]) ? icon : "");
    g_config.apps[app_idx].configured = (name[0] != '\0' && path[0] != '\0');

    if (config_save(g_config_path, &g_config)) {
        set_status("Atalho salvo com sucesso.");
    } else {
        set_status("Erro ao salvar configuracao. Verifique as permissoes.");
    }

    // Refresh key grid and preview
    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
}

static void remove_selected_action(void) {
    if (g_selected_key < 6 || g_selected_key > 8) return;
    int app_idx = g_selected_key - 6;
    ZeroMemory(&g_config.apps[app_idx], sizeof(DeckApp));
    config_save(g_config_path, &g_config);
    update_inspector_ui();
    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
    set_status("Acao removida do slot.");
}

static void update_inspector_ui(void) {
    if (!g_insp_title_edit) return;

    if (g_selected_key >= 6 && g_selected_key <= 8) {
        int app_idx = g_selected_key - 6;
        const DeckApp *app = &g_config.apps[app_idx];
        ShowWindow(g_insp_title_edit, SW_SHOW);
        ShowWindow(g_insp_path_edit, SW_SHOW);
        ShowWindow(g_insp_path_browse, SW_SHOW);
        ShowWindow(g_insp_icon_edit, SW_SHOW);
        ShowWindow(g_insp_icon_browse, SW_SHOW);
        ShowWindow(g_insp_save_btn, SW_SHOW);
        ShowWindow(g_insp_remove_btn, SW_SHOW);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_vol_track, SW_HIDE);
        ShowWindow(g_insp_mute_btn, SW_HIDE);

        SetWindowTextA(g_insp_title_edit, app->name);
        SetWindowTextA(g_insp_path_edit, app->path);
        SetWindowTextA(g_insp_icon_edit, app->icon_path);
    } else if (g_selected_key == 0 || g_selected_key == 1 || g_selected_key == 5) {
        // Volume adjustment
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_icon_edit, SW_HIDE);
        ShowWindow(g_insp_icon_browse, SW_HIDE);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_vol_track, SW_SHOW);
        ShowWindow(g_insp_mute_btn, SW_HIDE);

        float level = 0.0f; int muted = 0;
        action_get_volume(&level, &muted);
        SendMessageA(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
    } else if (g_selected_key == 2) {
        // Mute
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_icon_edit, SW_HIDE);
        ShowWindow(g_insp_icon_browse, SW_HIDE);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_HIDE);
        ShowWindow(g_insp_vol_track, SW_SHOW);
        ShowWindow(g_insp_mute_btn, SW_SHOW);

        float level = 0.0f; int muted = 0;
        action_get_volume(&level, &muted);
        SendMessageA(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
        SetWindowTextA(g_insp_mute_btn, muted ? "Desmutar Audio" : "Mutar Audio");
    } else {
        // Media or other
        ShowWindow(g_insp_title_edit, SW_HIDE);
        ShowWindow(g_insp_path_edit, SW_HIDE);
        ShowWindow(g_insp_path_browse, SW_HIDE);
        ShowWindow(g_insp_icon_edit, SW_HIDE);
        ShowWindow(g_insp_icon_browse, SW_HIDE);
        ShowWindow(g_insp_save_btn, SW_HIDE);
        ShowWindow(g_insp_remove_btn, SW_HIDE);
        ShowWindow(g_insp_test_btn, SW_SHOW);
        ShowWindow(g_insp_vol_track, SW_HIDE);
        ShowWindow(g_insp_mute_btn, SW_HIDE);
    }

    if (g_main_window) InvalidateRect(g_main_window, NULL, FALSE);
}

// Layout sizing and positioning
static void layout_controls(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width < 600 || height < 400) return;

    int topbar_h = 52;
    int statusbar_h = 30;
    int sidebar_w = 200;
    int inspector_w = 320;
    int canvas_x = sidebar_w;
    int canvas_w = width - sidebar_w - inspector_w;
    int content_h = height - topbar_h - statusbar_h;

    // Sidebar Category Buttons
    int side_top = topbar_h + 46;
    for (int i = 0; i < 7; ++i) {
        HWND btn = GetDlgItem(hwnd, ID_LIB_FAVORITES + i);
        if (btn) MoveWindow(btn, 12, side_top + i * 40, sidebar_w - 24, 34, TRUE);
    }

    // 15 Keys Grid (5 cols x 3 rows) in Canvas
    int grid_cols = 5;
    int grid_rows = 3;
    int gap = 14;
    int max_key_w = (canvas_w - 60 - (grid_cols - 1) * gap) / grid_cols;
    int max_key_h = (content_h - 120 - (grid_rows - 1) * gap) / grid_rows;
    int key_size = max_key_w < max_key_h ? max_key_w : max_key_h;
    if (key_size > 112) key_size = 112;
    if (key_size < 64) key_size = 64;

    int grid_total_w = grid_cols * key_size + (grid_cols - 1) * gap;
    int grid_total_h = grid_rows * key_size + (grid_rows - 1) * gap;
    int grid_start_x = canvas_x + (canvas_w - grid_total_w) / 2;
    int grid_start_y = topbar_h + 54 + (content_h - 100 - grid_total_h) / 2;

    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            int idx = row * grid_cols + col;
            int kx = grid_start_x + col * (key_size + gap);
            int ky = grid_start_y + row * (key_size + gap);
            if (g_keys[idx]) MoveWindow(g_keys[idx], kx, ky, key_size, key_size, TRUE);
        }
    }

    // Property Inspector Controls
    int insp_x = width - inspector_w + 20;
    int insp_w = inspector_w - 40;
    int iy = topbar_h + 74;

    if (g_insp_title_edit) MoveWindow(g_insp_title_edit, insp_x, iy, insp_w, 28, TRUE);
    iy += 50;
    if (g_insp_path_edit) MoveWindow(g_insp_path_edit, insp_x, iy, insp_w - 36, 28, TRUE);
    if (g_insp_path_browse) MoveWindow(g_insp_path_browse, insp_x + insp_w - 32, iy, 32, 28, TRUE);
    iy += 50;
    if (g_insp_icon_edit) MoveWindow(g_insp_icon_edit, insp_x, iy, insp_w - 36, 28, TRUE);
    if (g_insp_icon_browse) MoveWindow(g_insp_icon_browse, insp_x + insp_w - 32, iy, 32, 28, TRUE);
    iy += 44;
    if (g_insp_save_btn) MoveWindow(g_insp_save_btn, insp_x, iy, insp_w, 36, TRUE);
    iy += 42;
    if (g_insp_remove_btn) MoveWindow(g_insp_remove_btn, insp_x, iy, insp_w, 30, TRUE);

    // Audio Inspector Controls
    if (g_insp_vol_track) MoveWindow(g_insp_vol_track, insp_x, topbar_h + 90, insp_w, 28, TRUE);
    if (g_insp_mute_btn) MoveWindow(g_insp_mute_btn, insp_x, topbar_h + 130, insp_w, 34, TRUE);
    if (g_insp_test_btn) MoveWindow(g_insp_test_btn, insp_x, topbar_h + 174, insp_w, 34, TRUE);

    // Top Bar Right Settings Button
    HWND settings = GetDlgItem(hwnd, ID_SETTINGS);
    if (settings) MoveWindow(settings, width - 116, 10, 100, 32, TRUE);

    // Status bar at bottom
    if (g_status) MoveWindow(g_status, 16, height - statusbar_h + 5, width - 32, 20, TRUE);
}

// Drawing Functions
static void draw_key(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    int key_id = draw->CtlID;
    int idx = key_id - ID_KEY_BASE;
    BOOL is_selected = (idx == g_selected_key);
    BOOL is_pressed = (draw->itemState & ODS_SELECTED) != 0;

    SetBkMode(dc, TRANSPARENT);

    // Key base fill & border
    COLORREF bg_col = is_pressed ? RGB(10, 12, 15) : (is_selected ? RGB(18, 22, 28) : COLOR_KEY);
    COLORREF bdr_col = is_selected ? COLOR_ACCENT_BLUE : COLOR_BORDER;
    int bdr_thick = is_selected ? 2 : 1;

    HBRUSH brush = CreateSolidBrush(bg_col);
    HPEN pen = CreatePen(PS_SOLID, bdr_thick, bdr_col);
    HGDIOBJ old_b = SelectObject(dc, brush);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 14, 14);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(brush);
    DeleteObject(pen);

    RECT content = rect;
    if (is_pressed) OffsetRect(&content, 0, 1);

    // Draw Icon and Title based on key index
    const wchar_t *glyph = NULL;
    const char *title = "";
    HICON app_icon = NULL;
    BOOL owns_icon = FALSE;

    switch (idx) {
        case 0: glyph = L"＋"; title = "Volume +"; break;
        case 1: glyph = L"◖"; title = "Mudo"; break;
        case 2: glyph = L"▶"; title = "Play / Pause"; break;
        case 3: glyph = L"◀"; title = "Anterior"; break;
        case 4: glyph = L"▶"; title = "Próxima"; break;
        case 5: glyph = L"−"; title = "Volume -"; break;
        case 6: // App 1
            if (g_config.apps[0].configured) title = g_config.apps[0].name;
            else title = "Navegador";
            break;
        case 7: // App 2
            if (g_config.apps[1].configured) title = g_config.apps[1].name;
            else title = "Discord";
            break;
        case 8: // App 3
            if (g_config.apps[2].configured) title = g_config.apps[2].name;
            else title = "OBS Studio";
            break;
        case 9: glyph = L"📁"; title = "Pasta"; break;
        case 10: glyph = L"⌨"; title = "Atalho"; break;
        default:
            title = ""; // Empty slot
            break;
    }

    // Check custom icon for configured apps
    if (idx >= 6 && idx <= 8) {
        int app_idx = idx - 6;
        const DeckApp *app = &g_config.apps[app_idx];
        if (app->icon_path[0]) {
            app_icon = (HICON)LoadImageA(NULL, app->icon_path, IMAGE_ICON, 36, 36, LR_LOADFROMFILE);
            owns_icon = (app_icon != NULL);
        }
        if (!app_icon && app->path[0]) {
            SHFILEINFOA info;
            ZeroMemory(&info, sizeof(info));
            if (SHGetFileInfoA(app->path, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON)) {
                app_icon = info.hIcon;
                owns_icon = (app_icon != NULL);
            }
        }
    }

    // Render center graphic
    int center_y = content.top + (content.bottom - content.top) / 2 - 8;
    if (app_icon) {
        int sz = 36;
        DrawIconEx(dc, content.left + (content.right - content.left - sz) / 2, content.top + 14, app_icon, sz, sz, 0, NULL, DI_NORMAL);
        if (owns_icon) DestroyIcon(app_icon);
    } else if (glyph) {
        HGDIOBJ old_f = SelectObject(dc, g_font_glyph);
        SetTextColor(dc, (idx == 1) ? COLOR_WARNING : COLOR_TEXT);
        RECT gr = { content.left, content.top + 10, content.right, center_y + 14 };
        DrawTextW(dc, glyph, -1, &gr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_f);
    } else if (idx >= 6 && idx <= 8) {
        // App icon placeholder (Elgato circle style)
        HPEN circle_pen = CreatePen(PS_SOLID, 2, COLOR_ACCENT_BLUE);
        HGDIOBJ old_p2 = SelectObject(dc, circle_pen);
        HGDIOBJ old_b2 = SelectObject(dc, GetStockObject(NULL_BRUSH));
        int cx = (content.left + content.right) / 2;
        int cy = content.top + 32;
        Ellipse(dc, cx - 16, cy - 16, cx + 16, cy + 16);
        Ellipse(dc, cx - 7, cy - 7, cx + 7, cy + 7);
        SelectObject(dc, old_b2);
        SelectObject(dc, old_p2);
        DeleteObject(circle_pen);
    } else {
        // Empty slot plus sign
        HGDIOBJ old_f = SelectObject(dc, g_font_label);
        SetTextColor(dc, RGB(70, 77, 88));
        RECT er = { content.left, content.top, content.right, content.bottom };
        DrawTextA(dc, "＋", -1, &er, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_f);
    }

    // Render label in lower area
    if (title && title[0]) {
        HGDIOBJ old_f = SelectObject(dc, g_font_label);
        SetTextColor(dc, is_selected ? COLOR_TEXT : COLOR_TEXT_MUTED);
        RECT tr = { content.left + 4, content.bottom - 24, content.right - 4, content.bottom - 4 };
        DrawTextA(dc, title, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old_f);
    }
}

static void draw_sidebar_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    int id = draw->CtlID;
    BOOL is_active = (id == g_selected_category);

    COLORREF bg = is_active ? COLOR_PANEL_ELEV : COLOR_PANEL;
    HBRUSH brush = CreateSolidBrush(bg);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    if (is_active) {
        // Active indicator on left
        RECT bar = { rect.left, rect.top + 4, rect.left + 3, rect.bottom - 4 };
        HBRUSH acc = CreateSolidBrush(COLOR_ACCENT_BLUE);
        FillRect(dc, &bar, acc);
        DeleteObject(acc);
    }

    char text[64];
    GetWindowTextA(draw->hwndItem, text, sizeof(text));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, is_active ? COLOR_TEXT : COLOR_TEXT_MUTED);
    HGDIOBJ old_f = SelectObject(dc, g_font_label);
    RECT tr = { rect.left + 16, rect.top, rect.right - 8, rect.bottom };
    DrawTextA(dc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_f);
}

static void draw_inspector_preview(HDC dc, RECT rect) {
    HBRUSH br = CreateSolidBrush(COLOR_KEY);
    HPEN pen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    HGDIOBJ old_b = SelectObject(dc, br);
    HGDIOBJ old_p = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(br);
    DeleteObject(pen);

    const char *title = "Preview";
    if (g_selected_key >= 6 && g_selected_key <= 8) {
        int app_idx = g_selected_key - 6;
        if (g_config.apps[app_idx].configured) title = g_config.apps[app_idx].name;
        else title = "OBS Studio";
    }

    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ old_f = SelectObject(dc, g_font_label);
    SetTextColor(dc, COLOR_TEXT);
    RECT tr = { rect.left + 4, rect.bottom - 22, rect.right - 4, rect.bottom - 4 };
    DrawTextA(dc, title, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Icon in preview
    HPEN p2 = CreatePen(PS_SOLID, 2, COLOR_ACCENT_BLUE);
    SelectObject(dc, p2);
    int cx = (rect.left + rect.right) / 2;
    int cy = rect.top + 28;
    Ellipse(dc, cx - 12, cy - 12, cx + 12, cy + 12);
    DeleteObject(p2);
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
            HINSTANCE inst = ((LPCREATESTRUCTA)lparam)->hInstance;
            g_wm_close_msg = RegisterWindowMessageA("CDeck_CloseMessage");

            // Top Bar Profile Selector Button
            CreateWindowA("BUTTON", "Default Profile ▼", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                140, 10, 140, 32, hwnd, (HMENU)ID_PROFILE_BTN, inst, NULL);

            // Top Bar Settings Button
            CreateWindowA("BUTTON", "⚙ Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 100, 32, hwnd, (HMENU)ID_SETTINGS, inst, NULL);

            // Action Library Sidebar Buttons
            const char *categories[] = {
                "⭐  Favoritas",
                "🗔  Sistema",
                "🔊  Áudio",
                "🎵  Mídia",
                "🗖  Aplicativos",
                "↔  Navegação",
                "🗘  Multi Action"
            };
            for (int i = 0; i < 7; ++i) {
                CreateWindowA("BUTTON", categories[i], WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    0, 0, 100, 34, hwnd, (HMENU)(INT_PTR)(ID_LIB_FAVORITES + i), inst, NULL);
            }

            // 15 Keys in Canvas
            for (int i = 0; i < ID_KEY_COUNT; ++i) {
                g_keys[i] = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    0, 0, 96, 96, hwnd, (HMENU)(INT_PTR)(ID_KEY_BASE + i), inst, NULL);
            }

            // Property Inspector Controls
            g_insp_title_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_TITLE, inst, NULL);
            g_insp_path_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_PATH, inst, NULL);
            g_insp_path_browse = CreateWindowA("BUTTON", "...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 32, 28, hwnd, (HMENU)ID_INSP_BROWSE_PATH, inst, NULL);
            g_insp_icon_edit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_ICON, inst, NULL);
            g_insp_icon_browse = CreateWindowA("BUTTON", "...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 32, 28, hwnd, (HMENU)ID_INSP_BROWSE_ICON, inst, NULL);
            g_insp_save_btn = CreateWindowA("BUTTON", "Salvar alterações", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 100, 36, hwnd, (HMENU)ID_INSP_SAVE, inst, NULL);
            g_insp_remove_btn = CreateWindowA("BUTTON", "Remover ação", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 100, 30, hwnd, (HMENU)ID_INSP_REMOVE, inst, NULL);

            // Audio inspector controls
            g_insp_vol_track = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                WS_CHILD | TBS_NOTICKS | TBS_HORZ,
                0, 0, 100, 28, hwnd, (HMENU)ID_INSP_VOL_TRACK, inst, NULL);
            if (g_insp_vol_track) {
                SendMessageA(g_insp_vol_track, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            }
            g_insp_mute_btn = CreateWindowA("BUTTON", "Alternar Mudo", WS_CHILD | BS_PUSHBUTTON,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_MUTE_BTN, inst, NULL);
            g_insp_test_btn = CreateWindowA("BUTTON", "Executar ação", WS_CHILD | BS_PUSHBUTTON,
                0, 0, 100, 34, hwnd, (HMENU)ID_INSP_TEST, inst, NULL);

            // Status bar at bottom
            g_status = CreateWindowA("STATIC", "Pronto. C-Deck Elgato Edition.",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                16, 0, 400, 20, hwnd, (HMENU)ID_STATUS, inst, NULL);
            if (g_status && g_font_sub) SendMessageA(g_status, WM_SETFONT, (WPARAM)g_font_sub, TRUE);

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
            int statusbar_h = 30;
            int sidebar_w = 200;
            int inspector_w = 320;
            int canvas_x = sidebar_w;
            int canvas_w = width - sidebar_w - inspector_w;

            SetBkMode(dc, TRANSPARENT);

            // 1. Top Bar Background
            RECT top_rect = { 0, 0, width, topbar_h };
            FillRect(dc, &top_rect, g_brush_bg);
            HPEN div_pen = CreatePen(PS_SOLID, 1, COLOR_BORDER_SUBTLE);
            HGDIOBJ old_pen = SelectObject(dc, div_pen);
            MoveToEx(dc, 0, topbar_h - 1, NULL); LineTo(dc, width, topbar_h - 1);

            // Top Bar Brand Logo and Title
            HGDIOBJ old_f = SelectObject(dc, g_font_title);
            SetTextColor(dc, COLOR_TEXT);
            RECT logo_rect = { 20, 0, 130, topbar_h };
            DrawTextA(dc, "C-DECK", -1, &logo_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // 2. Action Library Sidebar (Left)
            RECT side_rect = { 0, topbar_h, sidebar_w, height - statusbar_h };
            FillRect(dc, &side_rect, g_brush_panel);
            MoveToEx(dc, sidebar_w - 1, topbar_h, NULL); LineTo(dc, sidebar_w - 1, height - statusbar_h);

            SelectObject(dc, g_font_header);
            SetTextColor(dc, COLOR_TEXT_MUTED);
            RECT side_title = { 16, topbar_h + 16, sidebar_w - 16, topbar_h + 36 };
            DrawTextA(dc, "AÇÕES", -1, &side_title, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // 3. Central Canvas
            RECT can_rect = { canvas_x, topbar_h, canvas_x + canvas_w, height - statusbar_h };
            FillRect(dc, &can_rect, g_brush_bg);

            // Canvas Header: LAYOUT DO DISPOSITIVO / Stream Deck · 15 teclas
            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_MUTED);
            RECT dev_lbl = { canvas_x + 30, topbar_h + 14, canvas_x + canvas_w, topbar_h + 30 };
            DrawTextA(dc, "LAYOUT DO DISPOSITIVO", -1, &dev_lbl, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(dc, g_font_header);
            SetTextColor(dc, COLOR_TEXT);
            RECT dev_name = { canvas_x + 30, topbar_h + 30, canvas_x + canvas_w, topbar_h + 50 };
            DrawTextA(dc, "Stream Deck · 15 teclas", -1, &dev_name, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Pagination at canvas bottom
            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_MUTED);
            RECT pag_rect = { canvas_x, height - statusbar_h - 26, canvas_x + canvas_w, height - statusbar_h - 8 };
            DrawTextA(dc, "Páginas  1  2  3  +", -1, &pag_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // 4. Property Inspector (Right Panel)
            int insp_x = width - inspector_w;
            RECT insp_rect = { insp_x, topbar_h, width, height - statusbar_h };
            FillRect(dc, &insp_rect, g_brush_panel);
            MoveToEx(dc, insp_x, topbar_h, NULL); LineTo(dc, insp_x, height - statusbar_h);

            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_MUTED);
            RECT pr_hdr = { insp_x + 20, topbar_h + 14, width - 20, topbar_h + 28 };
            DrawTextA(dc, "PROPRIEDADES", -1, &pr_hdr, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(dc, g_font_header);
            SetTextColor(dc, COLOR_TEXT);
            RECT pr_type = { insp_x + 20, topbar_h + 30, width - 20, topbar_h + 50 };
            const char *act_hdr = (g_selected_key >= 6 && g_selected_key <= 8) ? "Sistema: Atalho"
                : ((g_selected_key <= 1 || g_selected_key == 5) ? "Sistema: Controle de Áudio"
                : ((g_selected_key == 2) ? "Sistema: Mute" : "Sistema: Multimídia"));
            DrawTextA(dc, act_hdr, -1, &pr_type, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Labels inside Property Inspector for configured app slots
            if (g_selected_key >= 6 && g_selected_key <= 8) {
                SelectObject(dc, g_font_label);
                SetTextColor(dc, COLOR_TEXT_MUTED);
                RECT l1 = { insp_x + 20, topbar_h + 58, width - 20, topbar_h + 72 };
                DrawTextA(dc, "Título", -1, &l1, DT_LEFT | DT_SINGLELINE);
                RECT l2 = { insp_x + 20, topbar_h + 108, width - 20, topbar_h + 122 };
                DrawTextA(dc, "Aplicativo / arquivo / URL", -1, &l2, DT_LEFT | DT_SINGLELINE);
                RECT l3 = { insp_x + 20, topbar_h + 158, width - 20, topbar_h + 172 };
                DrawTextA(dc, "Ícone (.ico opcional)", -1, &l3, DT_LEFT | DT_SINGLELINE);

                // Key Preview Section at bottom of Inspector
                RECT prev_hdr = { insp_x + 20, height - statusbar_h - 110, width - 20, height - statusbar_h - 96 };
                DrawTextA(dc, "Prévia da tecla", -1, &prev_hdr, DT_LEFT | DT_SINGLELINE);
                RECT prev_box = { insp_x + 20, height - statusbar_h - 92, insp_x + 92, height - statusbar_h - 20 };
                draw_inspector_preview(dc, prev_box);
            }

            // 5. Status Bar at Bottom
            RECT stat_rect = { 0, height - statusbar_h, width, height };
            FillRect(dc, &stat_rect, g_brush_bg);
            MoveToEx(dc, 0, height - statusbar_h, NULL); LineTo(dc, width, height - statusbar_h);

            SelectObject(dc, g_font_sub);
            SetTextColor(dc, COLOR_TEXT_MUTED);
            RECT info_rect = { width - 220, height - statusbar_h, width - 16, height };
            DrawTextA(dc, "Windows Core Audio · C++ Win32", -1, &info_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(dc, old_pen);
            DeleteObject(div_pen);
            SelectObject(dc, old_f);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DRAWITEM: {
            const DRAWITEMSTRUCT *draw = (const DRAWITEMSTRUCT *)lparam;
            if (draw->CtlID >= ID_KEY_BASE && draw->CtlID < ID_KEY_BASE + ID_KEY_COUNT) {
                draw_key(draw);
                return TRUE;
            }
            if (draw->CtlID >= ID_LIB_FAVORITES && draw->CtlID <= ID_LIB_MULTIACTION) {
                draw_sidebar_button(draw);
                return TRUE;
            }
            break;
        }

        case WM_CTLCOLOREDIT: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, COLOR_TEXT);
            SetBkColor(dc, COLOR_PANEL_ELEV);
            return (LRESULT)g_brush_elevated;
        }

        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, COLOR_TEXT_MUTED);
            SetBkColor(dc, COLOR_BG);
            return (LRESULT)g_brush_bg;
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);

            // Handle Key Clicks in Grid
            if (id >= ID_KEY_BASE && id < ID_KEY_BASE + ID_KEY_COUNT) {
                int clicked = id - ID_KEY_BASE;
                g_selected_key = clicked;
                update_inspector_ui();
                execute_key_action(clicked);
                return 0;
            }

            // Handle Sidebar Category Clicks
            if (id >= ID_LIB_FAVORITES && id <= ID_LIB_MULTIACTION) {
                g_selected_category = id;
                InvalidateRect(hwnd, NULL, FALSE);
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

            if (id == ID_INSP_TEST) {
                execute_key_action(g_selected_key);
                return 0;
            }

            if (id == ID_INSP_MUTE_BTN) {
                action_toggle_mute();
                update_inspector_ui();
                return 0;
            }

            if (id == ID_INSP_BROWSE_PATH) {
                browse_file_for_inspector(hwnd, 0);
                return 0;
            }

            if (id == ID_INSP_BROWSE_ICON) {
                browse_file_for_inspector(hwnd, 1);
                return 0;
            }

            if (id == ID_SETTINGS || id == ID_PROFILE_BTN) {
                set_status("Configuração de perfis ativa. Perfil atual: Default Profile.");
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
                int pos = (int)SendMessageA(g_insp_vol_track, TBM_GETPOS, 0, 0);
                action_set_volume((float)pos / 100.0f);
                char msg[64];
                snprintf(msg, sizeof(msg), "Volume ajustado: %d%%.", pos);
                set_status(msg);
            }
            return 0;

        case WM_TIMER:
            if (wparam == ID_VOLUME_TIMER && GetCapture() != g_insp_vol_track) {
                float level = 0.0f; int muted = 0;
                if (action_get_volume(&level, &muted) == ACTION_SUCCESS && g_insp_vol_track) {
                    SendMessageA(g_insp_vol_track, TBM_SETPOS, TRUE, (int)(level * 100.0f + 0.5f));
                }
            }
            return 0;

        case WM_HOTKEY:
            if (wparam == ID_HOTKEY_UP) report_result(action_volume_step(0.05f), "Volume aumentado (+5%).");
            else if (wparam == ID_HOTKEY_DOWN) report_result(action_volume_step(-0.05f), "Volume reduzido (−5%).");
            else if (wparam == ID_HOTKEY_MUTE) report_result(action_toggle_mute(), "Mute alternado.");
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
                set_status("Nao foi possivel minimizar para a bandeja.");
                MessageBoxA(hwnd, "O Windows nao permitiu criar o icone da bandeja. O C-Deck continuara aberto.", "C-Deck", MB_OK | MB_ICONWARNING);
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
            if (g_font_glyph) DeleteObject(g_font_glyph);
            if (g_font_btn) DeleteObject(g_font_btn);

            if (g_brush_bg) DeleteObject(g_brush_bg);
            if (g_brush_panel) DeleteObject(g_brush_panel);
            if (g_brush_elevated) DeleteObject(g_brush_elevated);
            if (g_brush_key) DeleteObject(g_brush_key);
            if (g_brush_accent) DeleteObject(g_brush_accent);

            g_font_title = NULL;
            g_font_header = NULL;
            g_font_label = NULL;
            g_font_sub = NULL;
            g_font_glyph = NULL;
            g_font_btn = NULL;

            g_brush_bg = NULL;
            g_brush_panel = NULL;
            g_brush_elevated = NULL;
            g_brush_key = NULL;
            g_brush_accent = NULL;

            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int interface_run(HINSTANCE instance, int show_command, const DeckConfig *config) {
    WNDCLASSEXA window_class;
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

    // Typography
    g_font_title = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_font_header = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_font_label = CreateFontA(12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_font_sub = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_font_glyph = CreateFontW(28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    g_font_btn = CreateFontA(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    // Brushes
    g_brush_bg = CreateSolidBrush(COLOR_BG);
    g_brush_panel = CreateSolidBrush(COLOR_PANEL);
    g_brush_elevated = CreateSolidBrush(COLOR_PANEL_ELEV);
    g_brush_key = CreateSolidBrush(COLOR_KEY);
    g_brush_accent = CreateSolidBrush(COLOR_ACCENT_BLUE);

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_APP_ICON));
    window_class.hIconSm = window_class.hIcon;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = g_brush_bg;
    window_class.lpszClassName = CLASS_NAME;
    if (!RegisterClassExA(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    // Desktop Elgato Stream Deck dimensions: 1200 x 760
    hwnd = CreateWindowExA(
        0, CLASS_NAME, "C-DECK | Elgato Stream Deck Edition",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 760,
        NULL, NULL, instance, NULL
    );
    if (!hwnd) return 1;
    g_main_window = hwnd;

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    g_main_window = NULL;
    return (int)message.wParam;
}
