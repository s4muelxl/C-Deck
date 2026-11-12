#include "interface.h"
#include "actions.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>

#define ID_VOL_UP 1001
#define ID_VOL_DOWN 1002
#define ID_MUTE 1003
#define ID_MEDIA_PLAY 1004
#define ID_MEDIA_NEXT 1005
#define ID_MEDIA_PREVIOUS 1006
#define ID_APP_1 1011
#define ID_SETTINGS 1020
#define ID_SAVE 1021
#define ID_CANCEL 1022
#define ID_STATUS 1099
#define ID_VOLUME_TRACK 1030
#define ID_VOLUME_LABEL 1031
#define ID_TRAY_OPEN 3001
#define ID_TRAY_EXIT 3002
#define ID_HOTKEY_UP 1
#define ID_HOTKEY_DOWN 2
#define ID_HOTKEY_MUTE 3
#define WM_TRAYICON (WM_APP + 1)
#define ID_VOLUME_TIMER 1
#define ID_NAME_BASE 2001
#define ID_PATH_BASE 2011
#define ID_APP_BROWSE_BASE 2021
#define ID_ICON_BASE 2031
#define ID_ICON_BROWSE_BASE 2041
#define ID_HEADER 1080
#define IDI_APP_ICON 101

static const char CLASS_NAME[] = "CDeckMainWindow";
static const char EDITOR_CLASS[] = "CDeckEditorWindow";
static DeckConfig g_config;
static char g_config_path[MAX_PATH];
static HWND g_main_window;
static HWND g_editor_window;
static HWND g_status;
static HWND g_editor_status;
static HFONT g_title_font;
static HFONT g_subtitle_font;
static HFONT g_badge_font;
static HFONT g_key_label_font;
static HFONT g_glyph_font;
static HFONT g_status_font;
static HBRUSH g_background;
static HBRUSH g_card_background;
static HBRUSH g_header_background;
static HBRUSH g_edit_background;
static HWND g_volume_track;
static HWND g_volume_label;
static HWND g_header;
static NOTIFYICONDATAA g_tray_data;
static int g_tray_added;
static UINT g_wm_close_msg;

static void update_volume_ui(void) {
    float level = 0.0f;
    int muted = 0;
    char label[64];
    if (!g_volume_track || !g_volume_label) return;
    if (action_get_volume(&level, &muted) != ACTION_SUCCESS) {
        SetWindowTextA(g_volume_label, "VOLUME DO SISTEMA: --");
        return;
    }
    {
        int percent = (int)(level * 100.0f + 0.5f);
        SendMessageA(g_volume_track, TBM_SETPOS, TRUE, percent);
        if (muted) {
            snprintf(label, sizeof(label), "VOLUME DO SISTEMA: %d%%  |  [ MUDO ATIVO ]", percent);
        } else {
            snprintf(label, sizeof(label), "VOLUME DO SISTEMA: %d%%", percent);
        }
        SetWindowTextA(g_volume_label, label);
        if (g_main_window) {
            HWND mute_btn = GetDlgItem(g_main_window, ID_MUTE);
            if (mute_btn) InvalidateRect(mute_btn, NULL, TRUE);
            InvalidateRect(g_volume_label, NULL, TRUE);
        }
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
    lstrcpynA(g_tray_data.szTip, "C-Deck - Stream Deck Controller", (int)sizeof(g_tray_data.szTip));
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

static void set_status(const char *message) {
    if (g_status) SetWindowTextA(g_status, message);
}

static void report_result(ActionResult result, const char *success_message) {
    switch (result) {
        case ACTION_SUCCESS: set_status(success_message); break;
        case ACTION_AUDIO_UNAVAILABLE: set_status("Dispositivo de audio indisponivel."); break;
        case ACTION_MEDIA_UNAVAILABLE: set_status("O Windows nao aceitou o comando de midia."); break;
        case ACTION_LAUNCH_FAILED: set_status("Nao foi possivel abrir o aplicativo. Confira o caminho."); break;
        case ACTION_NOT_CONFIGURED: set_status("Atalho nao configurado. Abra Configurar atalhos."); break;
        default: set_status("Nao foi possivel executar esta acao."); break;
    }
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

static HWND add_button(HWND parent, const char *label, int id, HINSTANCE instance) {
    HWND button = CreateWindowA(
        "BUTTON", label,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 100, 60, parent, (HMENU)(INT_PTR)id, instance, NULL
    );
    if (button && g_key_label_font) SendMessageA(button, WM_SETFONT, (WPARAM)g_key_label_font, TRUE);
    return button;
}

static HWND add_editor_control(HWND parent, const char *class_name, const char *label,
                               DWORD style, int x, int y, int width, int height,
                               int id, HINSTANCE instance) {
    HWND control = CreateWindowExA(
        0, class_name, label, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, (HMENU)(INT_PTR)id, instance, NULL
    );
    if (control && g_status_font) SendMessageA(control, WM_SETFONT, (WPARAM)g_status_font, TRUE);
    return control;
}

static void layout_controls(HWND hwnd) {
    RECT area;
    int width, height;
    int margin = 16, gap = 10;
    int header_h = 48;
    int vol_card_top = 58;
    int vol_card_h = 58;
    int footer_h = 40;

    GetClientRect(hwnd, &area);
    width = area.right - area.left;
    height = area.bottom - area.top;
    if (width < 320 || height < 420) return;

    if (g_header) MoveWindow(g_header, 0, 0, width, header_h, TRUE);

    int card_w = width - margin * 2;
    if (g_volume_label) MoveWindow(g_volume_label, margin + 14, vol_card_top + 6, card_w - 28, 18, TRUE);
    if (g_volume_track) MoveWindow(g_volume_track, margin + 10, vol_card_top + 28, card_w - 20, 24, TRUE);

    int grid_top = vol_card_top + vol_card_h + 12;
    int footer_y = height - margin - footer_h;
    int grid_bottom = footer_y - 10;
    int grid_h = grid_bottom - grid_top;
    int col_width = (width - margin * 2 - gap * 2) / 3;
    int row_height = (grid_h - gap * 2) / 3;

    if (col_width > 0 && row_height > 0) {
        // Row 0 (Audio): Vol Down, Mute, Vol Up
        HWND btn_vol_down = GetDlgItem(hwnd, ID_VOL_DOWN);
        HWND btn_mute = GetDlgItem(hwnd, ID_MUTE);
        HWND btn_vol_up = GetDlgItem(hwnd, ID_VOL_UP);

        int y0 = grid_top;
        if (btn_vol_down) MoveWindow(btn_vol_down, margin + 0 * (col_width + gap), y0, col_width, row_height, TRUE);
        if (btn_mute)     MoveWindow(btn_mute,     margin + 1 * (col_width + gap), y0, col_width, row_height, TRUE);
        if (btn_vol_up)   MoveWindow(btn_vol_up,   margin + 2 * (col_width + gap), y0, col_width, row_height, TRUE);

        // Row 1 (Media): Prev Track, Play/Pause, Next Track
        HWND btn_prev = GetDlgItem(hwnd, ID_MEDIA_PREVIOUS);
        HWND btn_play = GetDlgItem(hwnd, ID_MEDIA_PLAY);
        HWND btn_next = GetDlgItem(hwnd, ID_MEDIA_NEXT);

        int y1 = grid_top + 1 * (row_height + gap);
        if (btn_prev) MoveWindow(btn_prev, margin + 0 * (col_width + gap), y1, col_width, row_height, TRUE);
        if (btn_play) MoveWindow(btn_play, margin + 1 * (col_width + gap), y1, col_width, row_height, TRUE);
        if (btn_next) MoveWindow(btn_next, margin + 2 * (col_width + gap), y1, col_width, row_height, TRUE);

        // Row 2 (Apps): Slots 1, 2, 3
        int y2 = grid_top + 2 * (row_height + gap);
        for (int i = 0; i < DECK_APP_COUNT; ++i) {
            HWND btn_app = GetDlgItem(hwnd, ID_APP_1 + i);
            if (btn_app) MoveWindow(btn_app, margin + i * (col_width + gap), y2, col_width, row_height, TRUE);
        }
    }

    int settings_w = 160;
    if (g_status) MoveWindow(g_status, margin + 4, footer_y + 4, width - margin * 2 - settings_w - gap, footer_h - 8, TRUE);
    HWND settings = GetDlgItem(hwnd, ID_SETTINGS);
    if (settings) MoveWindow(settings, width - margin - settings_w, footer_y, settings_w, footer_h, TRUE);
}

static void draw_header(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    int height = rect.bottom - rect.top;
    HICON app_icon = (HICON)LoadImageA(
        GetModuleHandleA(NULL),
        MAKEINTRESOURCEA(IDI_APP_ICON),
        IMAGE_ICON, 28, 28, LR_SHARED
    );

    HBRUSH bg = g_header_background ? g_header_background : g_background;
    FillRect(dc, &rect, bg);

    // Separator line
    HPEN line_pen = CreatePen(PS_SOLID, 1, RGB(34, 40, 52));
    HGDIOBJ old_pen = SelectObject(dc, line_pen);
    MoveToEx(dc, rect.left, rect.bottom - 1, NULL);
    LineTo(dc, rect.right, rect.bottom - 1);
    SelectObject(dc, old_pen);
    DeleteObject(line_pen);

    // Official logo: loaded icon or geometric vector mark
    int icon_size = 28;
    int icon_x = rect.left + 16;
    int icon_y = rect.top + (height - icon_size) / 2;
    if (app_icon) {
        DrawIconEx(dc, icon_x, icon_y, app_icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
    } else {
        RECT tile_rect = { icon_x, icon_y, icon_x + icon_size, icon_y + icon_size };
        HBRUSH tile_brush = CreateSolidBrush(RGB(18, 22, 29));
        HPEN rim_pen = CreatePen(PS_SOLID, 1, RGB(125, 139, 160));
        HGDIOBJ old_b = SelectObject(dc, tile_brush);
        HGDIOBJ old_p = SelectObject(dc, rim_pen);
        RoundRect(dc, tile_rect.left, tile_rect.top, tile_rect.right, tile_rect.bottom, 8, 8);
        SelectObject(dc, old_b);
        SelectObject(dc, old_p);
        DeleteObject(tile_brush);
        DeleteObject(rim_pen);

        POINT c_pts[12] = {
            { icon_x + 20, icon_y + 7 },  { icon_x + 12, icon_y + 7 },
            { icon_x + 7,  icon_y + 12 }, { icon_x + 7,  icon_y + 16 },
            { icon_x + 12, icon_y + 21 }, { icon_x + 20, icon_y + 21 },
            { icon_x + 20, icon_y + 18 }, { icon_x + 13, icon_y + 18 },
            { icon_x + 10, icon_y + 15 }, { icon_x + 10, icon_y + 13 },
            { icon_x + 13, icon_y + 10 }, { icon_x + 20, icon_y + 10 }
        };
        HBRUSH silver = CreateSolidBrush(RGB(230, 235, 242));
        old_b = SelectObject(dc, silver);
        old_p = SelectObject(dc, GetStockObject(NULL_PEN));
        Polygon(dc, c_pts, 12);

        POINT lime_pts[4] = {
            { icon_x + 16, icon_y + 21 }, { icon_x + 21, icon_y + 21 },
            { icon_x + 24, icon_y + 18 }, { icon_x + 19, icon_y + 18 }
        };
        HBRUSH lime = CreateSolidBrush(RGB(157, 255, 0));
        SelectObject(dc, lime);
        Polygon(dc, lime_pts, 4);

        SelectObject(dc, old_b);
        SelectObject(dc, old_p);
        DeleteObject(silver);
        DeleteObject(lime);
    }

    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ old_font = SelectObject(dc, g_title_font);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT title_rect = { icon_x + icon_size + 12, rect.top + 6, icon_x + icon_size + 160, rect.top + 26 };
    DrawTextA(dc, "C-DECK", -1, &title_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, g_subtitle_font);
    SetTextColor(dc, RGB(125, 138, 155));
    RECT sub_rect = { icon_x + icon_size + 12, rect.top + 26, icon_x + icon_size + 200, rect.bottom - 4 };
    DrawTextA(dc, "STREAM DECK CONTROLLER", -1, &sub_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Status pill [ ● ATIVO ]
    int pill_w = 76;
    int pill_h = 24;
    int pill_r = rect.right - 16;
    int pill_l = pill_r - pill_w;
    int pill_t = rect.top + (height - pill_h) / 2;
    int pill_b = pill_t + pill_h;

    HBRUSH pill_brush = CreateSolidBrush(RGB(20, 26, 34));
    HPEN pill_pen = CreatePen(PS_SOLID, 1, RGB(46, 56, 70));
    HGDIOBJ old_b = SelectObject(dc, pill_brush);
    HGDIOBJ old_p = SelectObject(dc, pill_pen);
    RoundRect(dc, pill_l, pill_t, pill_r, pill_b, 12, 12);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(pill_brush);
    DeleteObject(pill_pen);

    HBRUSH dot_brush = CreateSolidBrush(RGB(157, 255, 0));
    old_b = SelectObject(dc, dot_brush);
    old_p = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, pill_l + 9, pill_t + 9, pill_l + 15, pill_t + 15);
    SelectObject(dc, old_b);
    SelectObject(dc, old_p);
    DeleteObject(dot_brush);

    SelectObject(dc, g_badge_font);
    SetTextColor(dc, RGB(157, 255, 0));
    RECT pill_text_rect = { pill_l + 18, pill_t, pill_r - 4, pill_b };
    DrawTextA(dc, "ATIVO", -1, &pill_text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, old_font);
}

static void draw_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    char label[DECK_TEXT_MAX + 24];
    BOOL pressed = (draw->itemState & ODS_SELECTED) != 0;
    BOOL focused = (draw->itemState & ODS_FOCUS) != 0;
    int control_id = GetDlgCtrlID(draw->hwndItem);
    int app_index = (control_id >= ID_APP_1 && control_id < ID_APP_1 + DECK_APP_COUNT)
        ? control_id - ID_APP_1 : -1;

    GetWindowTextA(draw->hwndItem, label, (int)sizeof(label));
    SetBkMode(dc, TRANSPARENT);

    // 1. FOOTER SETTINGS BUTTON / DIALOG BUTTONS
    if (control_id == ID_SETTINGS || control_id == ID_SAVE || control_id == ID_CANCEL) {
        COLORREF bg_color = (control_id == ID_SAVE)
            ? (pressed ? RGB(22, 42, 22) : RGB(30, 58, 32))
            : (pressed ? RGB(20, 24, 30) : RGB(26, 32, 42));
        COLORREF bdr_color = focused
            ? RGB(157, 255, 0)
            : ((control_id == ID_SAVE) ? RGB(157, 255, 0) : RGB(52, 62, 78));
        COLORREF txt_color = (control_id == ID_SAVE)
            ? RGB(157, 255, 0)
            : RGB(220, 226, 236);

        HBRUSH brush = CreateSolidBrush(bg_color);
        HPEN pen = CreatePen(PS_SOLID, 1, bdr_color);
        HGDIOBJ old_b = SelectObject(dc, brush);
        HGDIOBJ old_p = SelectObject(dc, pen);
        RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);
        SelectObject(dc, old_b);
        SelectObject(dc, old_p);
        DeleteObject(brush);
        DeleteObject(pen);

        RECT text_rect = rect;
        if (pressed) OffsetRect(&text_rect, 0, 1);
        HGDIOBJ old_f = SelectObject(dc, g_key_label_font);
        SetTextColor(dc, txt_color);
        DrawTextA(dc, label, -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_f);
        return;
    }

    // 2. STREAM DECK LCD KEYCAPS (3x3 GRID)
    float cur_vol = 0.0f;
    int is_muted = 0;
    action_get_volume(&cur_vol, &is_muted);
    BOOL mute_active = (control_id == ID_MUTE && is_muted);

    COLORREF fill = pressed
        ? RGB(12, 15, 20)
        : (focused ? RGB(26, 32, 42) : RGB(20, 24, 32));
    COLORREF border = focused
        ? RGB(157, 255, 0)
        : (mute_active ? RGB(255, 82, 82) : RGB(44, 52, 66));

    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, (focused || mute_active) ? 2 : 1, border);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 16, 16);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    RECT content_rect = rect;
    if (pressed) OffsetRect(&content_rect, 0, 2);

    // Top Slot Badge
    const char *badge_text = "";
    if (control_id == ID_VOL_DOWN) badge_text = "ÁUDIO · −5%";
    else if (control_id == ID_MUTE) badge_text = mute_active ? "ÁUDIO · MUDO" : "ÁUDIO · MUTE";
    else if (control_id == ID_VOL_UP) badge_text = "ÁUDIO · +5%";
    else if (control_id == ID_MEDIA_PREVIOUS) badge_text = "MÍDIA · ANTERIOR";
    else if (control_id == ID_MEDIA_PLAY) badge_text = "MÍDIA · PLAY/PAUSE";
    else if (control_id == ID_MEDIA_NEXT) badge_text = "MÍDIA · PRÓXIMA";
    else if (app_index >= 0) {
        const DeckApp *app = &g_config.apps[app_index];
        int is_web = app->configured && (_strnicmp(app->path, "http://", 7) == 0 || _strnicmp(app->path, "https://", 8) == 0);
        if (app_index == 0) badge_text = is_web ? "01 · WEB" : (app->configured ? "01 · APP" : "01 · LIVRE");
        else if (app_index == 1) badge_text = is_web ? "02 · WEB" : (app->configured ? "02 · APP" : "02 · LIVRE");
        else if (app_index == 2) badge_text = is_web ? "03 · WEB" : (app->configured ? "03 · APP" : "03 · LIVRE");
    }

    HGDIOBJ old_font = SelectObject(dc, g_badge_font);
    SetTextColor(dc, mute_active ? RGB(255, 120, 120) : RGB(120, 132, 148));
    RECT badge_rect = { content_rect.left + 8, content_rect.top + 8, content_rect.right - 8, content_rect.top + 22 };
    DrawTextA(dc, badge_text, -1, &badge_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Center Graphic (Glyph or Icon)
    RECT glyph_rect = { content_rect.left + 6, content_rect.top + 24, content_rect.right - 6, content_rect.bottom - 28 };
    if (app_index < 0) {
        const wchar_t *glyph = L"";
        if (control_id == ID_VOL_DOWN) glyph = L"−";
        else if (control_id == ID_MUTE) glyph = L"◖";
        else if (control_id == ID_VOL_UP) glyph = L"＋";
        else if (control_id == ID_MEDIA_PREVIOUS) glyph = L"«";
        else if (control_id == ID_MEDIA_PLAY) glyph = L"▶";
        else if (control_id == ID_MEDIA_NEXT) glyph = L"»";

        SelectObject(dc, g_glyph_font);
        SetTextColor(dc, mute_active ? RGB(255, 82, 82) : RGB(157, 255, 0));
        DrawTextW(dc, glyph, -1, &glyph_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        const DeckApp *app = &g_config.apps[app_index];
        HICON app_icon = NULL;
        BOOL owns_icon = FALSE;
        int is_web = app->configured && (_strnicmp(app->path, "http://", 7) == 0 || _strnicmp(app->path, "https://", 8) == 0);

        if (app->icon_path[0]) {
            app_icon = (HICON)LoadImageA(NULL, app->icon_path, IMAGE_ICON, 36, 36, LR_LOADFROMFILE);
            owns_icon = app_icon != NULL;
        }
        if (!app_icon && app->path[0] && !is_web) {
            SHFILEINFOA info;
            ZeroMemory(&info, sizeof(info));
            if (SHGetFileInfoA(app->path, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON)) {
                app_icon = info.hIcon;
                owns_icon = app_icon != NULL;
            }
        }

        if (app_icon) {
            int icon_size = 36;
            int icon_x = glyph_rect.left + ((glyph_rect.right - glyph_rect.left) - icon_size) / 2;
            int icon_y = glyph_rect.top + ((glyph_rect.bottom - glyph_rect.top) - icon_size) / 2;
            DrawIconEx(dc, icon_x, icon_y, app_icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
            if (owns_icon) DestroyIcon(app_icon);
        } else if (is_web) {
            HPEN globe_pen = CreatePen(PS_SOLID, 2, RGB(157, 255, 0));
            HGDIOBJ prev_pen = SelectObject(dc, globe_pen);
            HGDIOBJ prev_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            int cx = (glyph_rect.left + glyph_rect.right) / 2;
            int cy = (glyph_rect.top + glyph_rect.bottom) / 2;
            int r = 16;
            Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
            Arc(dc, cx - r / 2, cy - r, cx + r / 2, cy + r, cx, cy - r, cx, cy + r);
            MoveToEx(dc, cx - r, cy, NULL);
            LineTo(dc, cx + r, cy);
            SelectObject(dc, prev_brush);
            SelectObject(dc, prev_pen);
            DeleteObject(globe_pen);
        } else {
            SelectObject(dc, g_glyph_font);
            SetTextColor(dc, RGB(72, 82, 98));
            DrawTextW(dc, L"+", -1, &glyph_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    // Bottom Key Label
    const char *key_label = label;
    if (control_id == ID_VOL_DOWN) key_label = "VOLUME −";
    else if (control_id == ID_MUTE) key_label = mute_active ? "DESMUTAR" : "MUTAR";
    else if (control_id == ID_VOL_UP) key_label = "VOLUME ＋";
    else if (control_id == ID_MEDIA_PREVIOUS) key_label = "ANTERIOR";
    else if (control_id == ID_MEDIA_PLAY) key_label = "PLAY / PAUSE";
    else if (control_id == ID_MEDIA_NEXT) key_label = "PRÓXIMA";
    else if (app_index >= 0) {
        if (!g_config.apps[app_index].configured) key_label = "CONFIGURAR";
    }

    SelectObject(dc, g_key_label_font);
    SetTextColor(dc, (draw->itemState & ODS_DISABLED) ? RGB(110, 118, 128) : RGB(242, 245, 250));
    RECT bottom_rect = { content_rect.left + 6, content_rect.bottom - 26, content_rect.right - 6, content_rect.bottom - 6 };
    DrawTextA(dc, key_label, -1, &bottom_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(dc, old_font);
}

static void browse_editor_file(HWND hwnd, int index, int choose_icon) {
    OPENFILENAMEA dialog;
    char selected[DECK_TEXT_MAX];
    const char *filter = choose_icon
        ? "Icones do Windows (*.ico)\0*.ico\0Todos os arquivos (*.*)\0*.*\0\0"
        : "Aplicativos (*.exe;*.com;*.bat;*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Todos os arquivos (*.*)\0*.*\0\0";
    int control_id = choose_icon ? ID_ICON_BASE + index : ID_PATH_BASE + index;
    ZeroMemory(&dialog, sizeof(dialog));
    ZeroMemory(selected, sizeof(selected));
    GetWindowTextA(GetDlgItem(hwnd, control_id), selected, sizeof(selected));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = selected;
    dialog.nMaxFile = sizeof(selected);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = choose_icon ? "Escolher icone do aplicativo" : "Escolher aplicativo";
    if (GetOpenFileNameA(&dialog)) SetWindowTextA(GetDlgItem(hwnd, control_id), selected);
}

static void open_editor(HWND owner) {
    RECT bounds;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (g_editor_window) {
        SetForegroundWindow(g_editor_window);
        return;
    }
    if (GetWindowRect(owner, &bounds)) {
        x = bounds.left + ((bounds.right - bounds.left) - 900) / 2;
        y = bounds.top + ((bounds.bottom - bounds.top) - 520) / 2;
    }
    g_editor_window = CreateWindowExA(
        WS_EX_DLGMODALFRAME, EDITOR_CLASS, "C-DECK | Configurar atalhos",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, 900, 520, owner, NULL, (HINSTANCE)GetWindowLongPtrA(owner, GWLP_HINSTANCE), NULL
    );
    if (!g_editor_window) {
        set_status("Nao foi possivel abrir a configuracao.");
        return;
    }
    EnableWindow(owner, FALSE);
    ShowWindow(g_editor_window, SW_SHOW);
    UpdateWindow(g_editor_window);
    SetForegroundWindow(g_editor_window);
}

static void save_editor(HWND hwnd) {
    DeckConfig candidate = g_config;
    char name[DECK_TEXT_MAX];
    char path[DECK_TEXT_MAX];

    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        GetWindowTextA(GetDlgItem(hwnd, ID_NAME_BASE + i), name, sizeof(name));
        char icon_path[DECK_TEXT_MAX];
        GetWindowTextA(GetDlgItem(hwnd, ID_PATH_BASE + i), path, sizeof(path));
        GetWindowTextA(GetDlgItem(hwnd, ID_ICON_BASE + i), icon_path, sizeof(icon_path));
        {
            char *start = name;
            char *end;
            while (*start && isspace((unsigned char)*start)) ++start;
            memmove(name, start, strlen(start) + 1);
            end = name + strlen(name);
            while (end > name && isspace((unsigned char)end[-1])) *--end = '\0';
        }
        {
            char *start = path;
            char *end;
            while (*start && isspace((unsigned char)*start)) ++start;
            memmove(path, start, strlen(start) + 1);
            end = path + strlen(path);
            while (end > path && isspace((unsigned char)end[-1])) *--end = '\0';
        }
        if ((name[0] == '\0') != (path[0] == '\0')) {
            SetWindowTextA(g_editor_status, "Preencha nome e destino (executavel ou URL) ou deixe os dois vazios.");
            SetFocus(GetDlgItem(hwnd, ID_NAME_BASE + i));
            return;
        }
        strcpy(candidate.apps[i].name, name);
        strcpy(candidate.apps[i].path, path);
        strcpy(candidate.apps[i].icon_path, (name[0] && path[0]) ? icon_path : "");
        candidate.apps[i].configured = name[0] != '\0' && path[0] != '\0';
    }

    if (!config_save(g_config_path, &candidate)) {
        SetWindowTextA(g_editor_status, "Nao foi possivel salvar. Verifique a permissao da pasta.");
        return;
    }

    g_config = candidate;
    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        char label[DECK_TEXT_MAX + 24];
        if (g_config.apps[i].configured) {
            snprintf(label, sizeof(label), "%s", g_config.apps[i].name);
        } else {
            snprintf(label, sizeof(label), "CONFIGURAR APP %d", i + 1);
        }
        SetWindowTextA(GetDlgItem(g_main_window, ID_APP_1 + i), label);
    }
    set_status("Atalhos salvos com sucesso.");
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK editor_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE instance = ((LPCREATESTRUCTA)lparam)->hInstance;
            add_editor_control(hwnd, "STATIC", "Configure aplicativos ou sites. Para sites, informe uma URL completa iniciando com https://. O C-Deck abre no navegador padrao.",
                               SS_LEFT, 24, 18, 840, 24, 0, instance);
            add_editor_control(hwnd, "STATIC", "NOME", SS_LEFT, 24, 56, 160, 22, 0, instance);
            add_editor_control(hwnd, "STATIC", "EXECUTAVEL OU URL DO SITE", SS_LEFT, 195, 56, 360, 22, 0, instance);
            add_editor_control(hwnd, "STATIC", "ICONE PERSONALIZADO (.ICO) — OPCIONAL", SS_LEFT, 572, 56, 300, 22, 0, instance);
            for (int i = 0; i < DECK_APP_COUNT; ++i) {
                int top = 88 + i * 82;
                char row_label[32];
                char browse_label[] = "...";
                snprintf(row_label, sizeof(row_label), "ATALHO %02d", i + 1);
                add_editor_control(hwnd, "STATIC", row_label, SS_LEFT, 24, top, 160, 20, 0, instance);
                {
                    HWND name_edit = add_editor_control(hwnd, "EDIT", g_config.apps[i].name,
                        WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_BORDER, 24, top + 23, 160, 30,
                        ID_NAME_BASE + i, instance);
                    HWND path_edit = add_editor_control(hwnd, "EDIT", g_config.apps[i].path,
                        WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_BORDER, 195, top + 23, 330, 30,
                        ID_PATH_BASE + i, instance);
                    HWND path_browse = add_editor_control(hwnd, "BUTTON", browse_label,
                        WS_TABSTOP | BS_PUSHBUTTON, 530, top + 23, 34, 30, ID_APP_BROWSE_BASE + i, instance);
                    HWND icon_edit = add_editor_control(hwnd, "EDIT", g_config.apps[i].icon_path,
                        WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_BORDER, 572, top + 23, 245, 30,
                        ID_ICON_BASE + i, instance);
                    HWND icon_browse = add_editor_control(hwnd, "BUTTON", browse_label,
                        WS_TABSTOP | BS_PUSHBUTTON, 822, top + 23, 34, 30, ID_ICON_BROWSE_BASE + i, instance);
                    if (name_edit) SendMessageA(name_edit, EM_SETLIMITTEXT, DECK_TEXT_MAX - 1, 0);
                    if (path_edit) SendMessageA(path_edit, EM_SETLIMITTEXT, DECK_TEXT_MAX - 1, 0);
                    if (icon_edit) SendMessageA(icon_edit, EM_SETLIMITTEXT, DECK_TEXT_MAX - 1, 0);
                    (void)path_browse; (void)icon_browse;
                }
            }
            add_button(hwnd, "SALVAR", ID_SAVE, instance);
            add_button(hwnd, "CANCELAR", ID_CANCEL, instance);
            MoveWindow(GetDlgItem(hwnd, ID_SAVE), 640, 365, 100, 38, TRUE);
            MoveWindow(GetDlgItem(hwnd, ID_CANCEL), 752, 365, 110, 38, TRUE);
            g_editor_status = add_editor_control(hwnd, "STATIC",
                "Sites: cole a URL completa (https://...). Aplicativos: selecione o .exe. O icone .ico e opcional; vazio usa o icone original.",
                SS_LEFT | SS_CENTERIMAGE, 24, 430, 840, 34, 0, instance);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wparam) >= ID_APP_BROWSE_BASE && LOWORD(wparam) < ID_APP_BROWSE_BASE + DECK_APP_COUNT) {
                browse_editor_file(hwnd, LOWORD(wparam) - ID_APP_BROWSE_BASE, 0);
                return 0;
            }
            if (LOWORD(wparam) >= ID_ICON_BROWSE_BASE && LOWORD(wparam) < ID_ICON_BROWSE_BASE + DECK_APP_COUNT) {
                browse_editor_file(hwnd, LOWORD(wparam) - ID_ICON_BROWSE_BASE, 1);
                return 0;
            }
            if (LOWORD(wparam) == ID_SAVE) {
                save_editor(hwnd);
                return 0;
            }
            if (LOWORD(wparam) == ID_CANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(190, 196, 206));
            SetBkColor(dc, RGB(13, 16, 21));
            return (LRESULT)g_background;
        }
        case WM_CTLCOLOREDIT: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(242, 244, 247));
            SetBkColor(dc, RGB(26, 32, 42));
            return (LRESULT)g_edit_background;
        }
        case WM_DRAWITEM:
            draw_button((const DRAWITEMSTRUCT *)lparam);
            return TRUE;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_editor_window = NULL;
            g_editor_status = NULL;
            if (g_main_window) {
                EnableWindow(g_main_window, TRUE);
                SetForegroundWindow(g_main_window);
            }
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (g_wm_close_msg != 0 && message == g_wm_close_msg) {
        remove_tray_icon();
        DestroyWindow(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    switch (message) {
        case WM_CREATE: {
            HINSTANCE instance = ((LPCREATESTRUCTA)lparam)->hInstance;
            g_wm_close_msg = RegisterWindowMessageA("CDeck_CloseMessage");

            // Row 0: Audio keys
            add_button(hwnd, "VOLUME −", ID_VOL_DOWN, instance);
            add_button(hwnd, "MUTAR", ID_MUTE, instance);
            add_button(hwnd, "VOLUME ＋", ID_VOL_UP, instance);

            // Row 1: Media keys
            add_button(hwnd, "ANTERIOR", ID_MEDIA_PREVIOUS, instance);
            add_button(hwnd, "PLAY / PAUSE", ID_MEDIA_PLAY, instance);
            add_button(hwnd, "PRÓXIMA", ID_MEDIA_NEXT, instance);

            // Row 2: App keys
            for (int i = 0; i < DECK_APP_COUNT; ++i) {
                char label[DECK_TEXT_MAX + 24];
                if (g_config.apps[i].configured) {
                    snprintf(label, sizeof(label), "%s", g_config.apps[i].name);
                } else {
                    snprintf(label, sizeof(label), "CONFIGURAR APP %d", i + 1);
                }
                add_button(hwnd, label, ID_APP_1 + i, instance);
            }

            // Footer controls
            add_button(hwnd, "⚙ CONFIGURAR", ID_SETTINGS, instance);

            // Header (owner-drawn)
            g_header = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                0, 0, 480, 48, hwnd, (HMENU)(INT_PTR)ID_HEADER, instance, NULL);

            // Volume Touch Strip controls
            g_volume_label = CreateWindowA("STATIC", "VOLUME DO SISTEMA: --", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                30, 64, 420, 18, hwnd, (HMENU)(INT_PTR)ID_VOLUME_LABEL, instance, NULL);
            if (g_volume_label && g_badge_font) SendMessageA(g_volume_label, WM_SETFONT, (WPARAM)g_badge_font, TRUE);

            g_volume_track = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_NOTICKS | TBS_HORZ,
                26, 86, 428, 24, hwnd, (HMENU)(INT_PTR)ID_VOLUME_TRACK, instance, NULL);
            if (g_volume_track) {
                SendMessageA(g_volume_track, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
                SendMessageA(g_volume_track, TBM_SETPAGESIZE, 0, 5);
            }

            g_status = CreateWindowA(
                "STATIC", "Pronto. C-Deck ativo.",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                20, 0, 260, 34, hwnd, (HMENU)(INT_PTR)ID_STATUS, instance, NULL
            );
            if (g_status && g_status_font) SendMessageA(g_status, WM_SETFONT, (WPARAM)g_status_font, TRUE);

            layout_controls(hwnd);
            update_volume_ui();
            SetTimer(hwnd, ID_VOLUME_TIMER, 500, NULL);
            {
                int hotkeys_ok = 1;
                if (!RegisterHotKey(hwnd, ID_HOTKEY_UP, MOD_CONTROL | MOD_ALT, VK_UP)) hotkeys_ok = 0;
                if (!RegisterHotKey(hwnd, ID_HOTKEY_DOWN, MOD_CONTROL | MOD_ALT, VK_DOWN)) hotkeys_ok = 0;
                if (!RegisterHotKey(hwnd, ID_HOTKEY_MUTE, MOD_CONTROL | MOD_ALT, 'M')) hotkeys_ok = 0;
                if (!hotkeys_ok) set_status("Algum atalho global ja esta em uso por outro programa.");
            }
            return 0;
        }
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) {
                if (add_tray_icon(hwnd)) ShowWindow(hwnd, SW_HIDE);
                else { ShowWindow(hwnd, SW_RESTORE); set_status("Nao foi possivel criar o icone da bandeja."); }
                return 0;
            }
            layout_controls(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = 440;
            limits->ptMinTrackSize.y = 620;
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client;
            GetClientRect(hwnd, &client);

            int margin = 16;
            int vol_card_top = 58;
            int vol_card_h = 58;
            int card_right = client.right - margin;
            int card_bottom = vol_card_top + vol_card_h;

            if (card_right > margin && client.bottom > vol_card_top) {
                HBRUSH card_b = CreateSolidBrush(RGB(20, 24, 32));
                HPEN card_p = CreatePen(PS_SOLID, 1, RGB(38, 46, 58));
                HGDIOBJ old_b = SelectObject(dc, card_b);
                HGDIOBJ old_p = SelectObject(dc, card_p);
                RoundRect(dc, margin, vol_card_top, card_right, card_bottom, 14, 14);
                SelectObject(dc, old_b);
                SelectObject(dc, old_p);
                DeleteObject(card_b);
                DeleteObject(card_p);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DRAWITEM: {
            const DRAWITEMSTRUCT *draw = (const DRAWITEMSTRUCT *)lparam;
            if (draw->CtlID == ID_HEADER) {
                draw_header(draw);
                return TRUE;
            }
            draw_button(draw);
            return TRUE;
        }
        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            HWND ctl = (HWND)lparam;
            if (ctl == g_volume_label) {
                float level = 0.0f;
                int muted = 0;
                action_get_volume(&level, &muted);
                SetTextColor(dc, muted ? RGB(255, 82, 82) : RGB(157, 255, 0));
                SetBkColor(dc, RGB(20, 24, 32));
                return (LRESULT)g_card_background;
            }
            SetTextColor(dc, RGB(160, 168, 180));
            SetBkColor(dc, RGB(13, 16, 21));
            return (LRESULT)g_background;
        }
        case WM_HSCROLL:
            if ((HWND)lparam == g_volume_track) {
                int position = (int)SendMessageA(g_volume_track, TBM_GETPOS, 0, 0);
                if (action_set_volume((float)position / 100.0f) == ACTION_SUCCESS) {
                    char message[64];
                    snprintf(message, sizeof(message), "Volume: %d%%.", position);
                    set_status(message);
                    update_volume_ui();
                } else {
                    set_status("Nao foi possivel alterar o volume.");
                }
            }
            return 0;
        case WM_TIMER:
            if (wparam == ID_VOLUME_TIMER && GetCapture() != g_volume_track) update_volume_ui();
            return 0;
        case WM_HOTKEY:
            if (wparam == ID_HOTKEY_UP) report_result(action_volume_step(0.05f), "Volume aumentado (+5%).");
            else if (wparam == ID_HOTKEY_DOWN) report_result(action_volume_step(-0.05f), "Volume reduzido (−5%).");
            else if (wparam == ID_HOTKEY_MUTE) report_result(action_toggle_mute(), "Mute alternado.");
            update_volume_ui();
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
        case WM_COMMAND:
            if (LOWORD(wparam) == ID_TRAY_OPEN) {
                remove_tray_icon();
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                return 0;
            }
            if (LOWORD(wparam) == ID_TRAY_EXIT) {
                remove_tray_icon();
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                return 0;
            }
            switch (LOWORD(wparam)) {
                case ID_VOL_UP:
                    report_result(action_volume_step(0.05f), "Volume aumentado (+5%).");
                    break;
                case ID_VOL_DOWN:
                    report_result(action_volume_step(-0.05f), "Volume reduzido (−5%).");
                    break;
                case ID_MUTE:
                    report_result(action_toggle_mute(), "Mute alternado.");
                    break;
                case ID_MEDIA_PLAY:
                    report_result(action_media_play_pause(), "Play / Pause enviado.");
                    break;
                case ID_MEDIA_NEXT:
                    report_result(action_media_next(), "Próxima faixa enviada.");
                    break;
                case ID_MEDIA_PREVIOUS:
                    report_result(action_media_previous(), "Faixa anterior enviada.");
                    break;
                case ID_APP_1:
                case ID_APP_1 + 1:
                case ID_APP_1 + 2: {
                    int index = LOWORD(wparam) - ID_APP_1;
                    report_result(action_launch_app(&g_config.apps[index]), "Atalho executado.");
                    break;
                }
                case ID_SETTINGS:
                    open_editor(hwnd);
                    break;
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
            if (g_title_font) DeleteObject(g_title_font);
            if (g_subtitle_font) DeleteObject(g_subtitle_font);
            if (g_badge_font) DeleteObject(g_badge_font);
            if (g_key_label_font) DeleteObject(g_key_label_font);
            if (g_glyph_font) DeleteObject(g_glyph_font);
            if (g_status_font) DeleteObject(g_status_font);
            if (g_background) DeleteObject(g_background);
            if (g_card_background) DeleteObject(g_card_background);
            if (g_header_background) DeleteObject(g_header_background);
            if (g_edit_background) DeleteObject(g_edit_background);
            g_title_font = NULL;
            g_subtitle_font = NULL;
            g_badge_font = NULL;
            g_key_label_font = NULL;
            g_glyph_font = NULL;
            g_status_font = NULL;
            g_background = NULL;
            g_card_background = NULL;
            g_header_background = NULL;
            g_edit_background = NULL;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int interface_run(HINSTANCE instance, int show_command, const DeckConfig *config) {
    WNDCLASSEXA window_class;
    WNDCLASSEXA editor_class;
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
    g_title_font = CreateFontA(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_subtitle_font = CreateFontA(10, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_badge_font = CreateFontA(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_key_label_font = CreateFontA(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_glyph_font = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    g_status_font = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_background = CreateSolidBrush(RGB(13, 16, 21));
    g_card_background = CreateSolidBrush(RGB(20, 24, 32));
    g_header_background = CreateSolidBrush(RGB(16, 20, 27));
    g_edit_background = CreateSolidBrush(RGB(26, 32, 42));

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_APP_ICON));
    window_class.hIconSm = window_class.hIcon;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = g_background;
    window_class.lpszClassName = CLASS_NAME;
    if (!RegisterClassExA(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    ZeroMemory(&editor_class, sizeof(editor_class));
    editor_class.cbSize = sizeof(editor_class);
    editor_class.lpfnWndProc = editor_proc;
    editor_class.hInstance = instance;
    editor_class.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_APP_ICON));
    editor_class.hIconSm = editor_class.hIcon;
    editor_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    editor_class.hbrBackground = g_background;
    editor_class.lpszClassName = EDITOR_CLASS;
    if (!RegisterClassExA(&editor_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    hwnd = CreateWindowExA(
        0, CLASS_NAME, "C-DECK | Stream Deck Controller",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 700,
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
