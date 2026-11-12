#include "interface.h"
#include "actions.h"
#include <windows.h>
#include <stdio.h>

#define ID_VOL_UP 1001
#define ID_VOL_DOWN 1002
#define ID_MUTE 1003
#define ID_APP_1 1011
#define ID_STATUS 1099

static const char CLASS_NAME[] = "CDeckMainWindow";
static DeckConfig g_config;
static HWND g_status;
static HFONT g_button_font;
static HFONT g_status_font;
static HBRUSH g_background;

static void set_status(const char *message) {
    if (g_status) SetWindowTextA(g_status, message);
}

static void report_result(ActionResult result, const char *success_message) {
    switch (result) {
        case ACTION_SUCCESS: set_status(success_message); break;
        case ACTION_AUDIO_UNAVAILABLE: set_status("Dispositivo de audio indisponivel."); break;
        case ACTION_LAUNCH_FAILED: set_status("Nao foi possivel abrir o aplicativo. Confira o caminho."); break;
        case ACTION_NOT_CONFIGURED: set_status("Atalho nao configurado. Edite config/apps.cfg."); break;
        default: set_status("Nao foi possivel executar esta acao."); break;
    }
}

static HWND add_button(HWND parent, const char *label, int id, HINSTANCE instance) {
    HWND button = CreateWindowA(
        "BUTTON", label,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 100, 60, parent, (HMENU)(INT_PTR)id, instance, NULL
    );
    if (button && g_button_font) SendMessageA(button, WM_SETFONT, (WPARAM)g_button_font, TRUE);
    return button;
}

static void layout_controls(HWND hwnd) {
    RECT area;
    int width, height, margin = 22, gap = 12, column_width, row_height, status_height = 34;
    GetClientRect(hwnd, &area);
    width = area.right - area.left;
    height = area.bottom - area.top;
    column_width = (width - (margin * 2) - (gap * 2)) / 3;
    row_height = (height - (margin * 2) - gap - status_height - 12) / 2;
    if (column_width < 1 || row_height < 1) return;

    for (int i = 0; i < 3; ++i) {
        int x = margin + i * (column_width + gap);
        HWND audio_button = GetDlgItem(hwnd, ID_VOL_UP + i);
        HWND app_button = GetDlgItem(hwnd, ID_APP_1 + i);
        if (audio_button) MoveWindow(audio_button, x, margin, column_width, row_height, TRUE);
        if (app_button) MoveWindow(app_button, x, margin + row_height + gap, column_width, row_height, TRUE);
    }
    if (g_status) MoveWindow(g_status, margin, height - margin - status_height, width - margin * 2, status_height, TRUE);
}

static void draw_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    BOOL pressed = (draw->itemState & ODS_SELECTED) != 0;
    BOOL focused = (draw->itemState & ODS_FOCUS) != 0;
    COLORREF fill = pressed ? RGB(48, 54, 64) : RGB(29, 33, 40);
    COLORREF border = focused ? RGB(205, 215, 230) : RGB(55, 62, 73);
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(242, 244, 247));
    InflateRect(&rect, -1, -1);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    if (draw->itemState & ODS_DISABLED) SetTextColor(dc, RGB(120, 126, 136));
    DrawTextA(dc, (const char *)draw->itemData, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (focused) {
        RECT focus = rect;
        InflateRect(&focus, -7, -7);
        DrawFocusRect(dc, &focus);
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE instance = ((LPCREATESTRUCTA)lparam)->hInstance;
            add_button(hwnd, "VOLUME +", ID_VOL_UP, instance);
            add_button(hwnd, "VOLUME -", ID_VOL_DOWN, instance);
            add_button(hwnd, "MUTAR AUDIO", ID_MUTE, instance);
            for (int i = 0; i < DECK_APP_COUNT; ++i) {
                char label[DECK_TEXT_MAX + 16];
                if (g_config.apps[i].configured) {
                    snprintf(label, sizeof(label), "%s", g_config.apps[i].name);
                } else {
                    snprintf(label, sizeof(label), "CONFIGURAR APP %d", i + 1);
                }
                add_button(hwnd, label, ID_APP_1 + i, instance);
            }
            g_status = CreateWindowA(
                "STATIC", "Pronto. Escolha uma acao.",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                22, 0, 400, 34, hwnd, (HMENU)(INT_PTR)ID_STATUS, instance, NULL
            );
            if (g_status && g_status_font) SendMessageA(g_status, WM_SETFONT, (WPARAM)g_status_font, TRUE);
            layout_controls(hwnd);
            return 0;
        }
        case WM_SIZE:
            layout_controls(hwnd);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = 560;
            limits->ptMinTrackSize.y = 340;
            return 0;
        }
        case WM_DRAWITEM:
            draw_button((const DRAWITEMSTRUCT *)lparam);
            return TRUE;
        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(166, 173, 184));
            SetBkColor(dc, RGB(14, 16, 20));
            return (LRESULT)g_background;
        }
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case ID_VOL_UP:
                    report_result(action_volume_step(0.05f), "Volume aumentado em 5%.");
                    break;
                case ID_VOL_DOWN:
                    report_result(action_volume_step(-0.05f), "Volume reduzido em 5%.");
                    break;
                case ID_MUTE:
                    report_result(action_toggle_mute(), "Estado do audio alternado.");
                    break;
                case ID_APP_1:
                case ID_APP_1 + 1:
                case ID_APP_1 + 2: {
                    int index = LOWORD(wparam) - ID_APP_1;
                    report_result(action_launch_app(&g_config.apps[index]), "Aplicativo iniciado.");
                    break;
                }
            }
            return 0;
        case WM_DESTROY:
            if (g_button_font) DeleteObject(g_button_font);
            if (g_status_font) DeleteObject(g_status_font);
            if (g_background) DeleteObject(g_background);
            g_button_font = NULL;
            g_status_font = NULL;
            g_background = NULL;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

int interface_run(HINSTANCE instance, int show_command, const DeckConfig *config) {
    WNDCLASSEXA window_class;
    HWND hwnd;
    MSG message;
    if (!config) return 1;
    g_config = *config;
    g_button_font = CreateFontA(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_status_font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_background = CreateSolidBrush(RGB(14, 16, 20));

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = g_background;
    window_class.lpszClassName = CLASS_NAME;
    if (!RegisterClassExA(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    hwnd = CreateWindowExA(
        0, CLASS_NAME, "C-DECK | Painel de atalhos",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 410,
        NULL, NULL, instance, NULL
    );
    if (!hwnd) return 1;
    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return (int)message.wParam;
}
