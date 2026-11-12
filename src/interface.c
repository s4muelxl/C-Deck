#include "interface.h"
#include "actions.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <commctrl.h>
#include <shellapi.h>

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

static const char CLASS_NAME[] = "CDeckMainWindow";
static const char EDITOR_CLASS[] = "CDeckEditorWindow";
static DeckConfig g_config;
static char g_config_path[MAX_PATH];
static HWND g_main_window;
static HWND g_editor_window;
static HWND g_status;
static HWND g_editor_status;
static HFONT g_button_font;
static HFONT g_status_font;
static HBRUSH g_background;
static HBRUSH g_edit_background;
static HWND g_volume_track;
static HWND g_volume_label;
static NOTIFYICONDATAA g_tray_data;
static int g_tray_added;

static void update_volume_ui(void) {
    float level = 0.0f;
    int muted = 0;
    char label[64];
    if (!g_volume_track || !g_volume_label) return;
    if (action_get_volume(&level, &muted) != ACTION_SUCCESS) {
        SetWindowTextA(g_volume_label, "VOLUME --");
        return;
    }
    {
        int percent = (int)(level * 100.0f + 0.5f);
        SendMessageA(g_volume_track, TBM_SETPOS, TRUE, percent);
        snprintf(label, sizeof(label), muted ? "VOLUME %d%% | MUDO" : "VOLUME %d%%", percent);
        SetWindowTextA(g_volume_label, label);
    }
}

static void add_tray_icon(HWND hwnd) {
    if (g_tray_added) return;
    ZeroMemory(&g_tray_data, sizeof(g_tray_data));
    g_tray_data.cbSize = sizeof(g_tray_data);
    g_tray_data.hWnd = hwnd;
    g_tray_data.uID = 1;
    g_tray_data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray_data.uCallbackMessage = WM_TRAYICON;
    g_tray_data.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    lstrcpynA(g_tray_data.szTip, "C-Deck - controles rápidos", (int)sizeof(g_tray_data.szTip));
    g_tray_added = Shell_NotifyIconA(NIM_ADD, &g_tray_data) ? 1 : 0;
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
    if (button && g_button_font) SendMessageA(button, WM_SETFONT, (WPARAM)g_button_font, TRUE);
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
    int width, height, margin = 22, gap = 10, column_width, row_height, status_height = 34, slider_area = 48;
    GetClientRect(hwnd, &area);
    width = area.right - area.left;
    height = area.bottom - area.top;
    column_width = (width - (margin * 2) - (gap * 2)) / 3;
    row_height = (height - (margin * 2) - (gap * 3) - status_height - slider_area - 12) / 3;
    if (column_width < 1 || row_height < 1) return;

    if (g_volume_label) MoveWindow(g_volume_label, margin, margin, 132, 28, TRUE);
    if (g_volume_track) MoveWindow(g_volume_track, margin + 132, margin - 2, width - margin * 2 - 132, 32, TRUE);

    for (int i = 0; i < 3; ++i) {
        int x = margin + i * (column_width + gap);
        HWND audio_button = GetDlgItem(hwnd, ID_VOL_UP + i);
        HWND media_button = GetDlgItem(hwnd, ID_MEDIA_PLAY + i);
        HWND app_button = GetDlgItem(hwnd, ID_APP_1 + i);
        if (audio_button) MoveWindow(audio_button, x, margin + slider_area, column_width, row_height, TRUE);
        if (media_button) MoveWindow(media_button, x, margin + slider_area + row_height + gap, column_width, row_height, TRUE);
        if (app_button) MoveWindow(app_button, x, margin + slider_area + (row_height + gap) * 2, column_width, row_height, TRUE);
    }
    if (g_status) MoveWindow(g_status, margin, height - margin - status_height,
                             width - margin * 2 - 150 - gap, status_height, TRUE);
    {
        HWND settings = GetDlgItem(hwnd, ID_SETTINGS);
        if (settings) MoveWindow(settings, width - margin - 150, height - margin - status_height,
                                  150, status_height, TRUE);
    }
}

static void draw_button(const DRAWITEMSTRUCT *draw) {
    HDC dc = draw->hDC;
    RECT rect = draw->rcItem;
    char label[DECK_TEXT_MAX + 24];
    BOOL pressed = (draw->itemState & ODS_SELECTED) != 0;
    BOOL focused = (draw->itemState & ODS_FOCUS) != 0;
    COLORREF fill = pressed ? RGB(48, 54, 64) : RGB(29, 33, 40);
    COLORREF border = focused ? RGB(205, 215, 230) : RGB(55, 62, 73);
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);

    GetWindowTextA(draw->hwndItem, label, (int)sizeof(label));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, (draw->itemState & ODS_DISABLED) ? RGB(120, 126, 136) : RGB(242, 244, 247));
    InflateRect(&rect, -1, -1);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(dc, old_brush);
    SelectObject(dc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    DrawTextA(dc, label, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (focused) {
        RECT focus = rect;
        InflateRect(&focus, -7, -7);
        DrawFocusRect(dc, &focus);
    }
}

static void open_editor(HWND owner) {
    RECT bounds;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (g_editor_window) {
        SetForegroundWindow(g_editor_window);
        return;
    }
    if (GetWindowRect(owner, &bounds)) {
        x = bounds.left + ((bounds.right - bounds.left) - 620) / 2;
        y = bounds.top + ((bounds.bottom - bounds.top) - 430) / 2;
    }
    g_editor_window = CreateWindowExA(
        WS_EX_DLGMODALFRAME, EDITOR_CLASS, "C-DECK | Configurar atalhos",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, 620, 430, owner, NULL, (HINSTANCE)GetWindowLongPtrA(owner, GWLP_HINSTANCE), NULL
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
        GetWindowTextA(GetDlgItem(hwnd, ID_PATH_BASE + i), path, sizeof(path));
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
            SetWindowTextA(g_editor_status, "Preencha nome e caminho ou deixe os dois vazios.");
            SetFocus(GetDlgItem(hwnd, ID_NAME_BASE + i));
            return;
        }
        strcpy(candidate.apps[i].name, name);
        strcpy(candidate.apps[i].path, path);
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
            add_editor_control(hwnd, "STATIC", "Configure os nomes e os caminhos dos tres atalhos.",
                               SS_LEFT, 24, 18, 560, 24, 0, instance);
            add_editor_control(hwnd, "STATIC", "NOME DO BOTAO", SS_LEFT, 24, 56, 210, 22, 0, instance);
            add_editor_control(hwnd, "STATIC", "CAMINHO OU COMANDO", SS_LEFT, 250, 56, 340, 22, 0, instance);
            for (int i = 0; i < DECK_APP_COUNT; ++i) {
                int top = 88 + i * 70;
                char row_label[32];
                snprintf(row_label, sizeof(row_label), "ATALHO %02d", i + 1);
                add_editor_control(hwnd, "STATIC", row_label, SS_LEFT, 24, top, 200, 20, 0, instance);
                {
                    HWND name_edit = add_editor_control(hwnd, "EDIT", g_config.apps[i].name,
                        WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_BORDER, 24, top + 23, 210, 30,
                        ID_NAME_BASE + i, instance);
                    HWND path_edit = add_editor_control(hwnd, "EDIT", g_config.apps[i].path,
                        WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_BORDER, 250, top + 23, 340, 30,
                        ID_PATH_BASE + i, instance);
                    if (name_edit) SendMessageA(name_edit, EM_SETLIMITTEXT, DECK_TEXT_MAX - 1, 0);
                    if (path_edit) SendMessageA(path_edit, EM_SETLIMITTEXT, DECK_TEXT_MAX - 1, 0);
                }
            }
            add_button(hwnd, "SALVAR", ID_SAVE, instance);
            add_button(hwnd, "CANCELAR", ID_CANCEL, instance);
            MoveWindow(GetDlgItem(hwnd, ID_SAVE), 360, 315, 110, 38, TRUE);
            MoveWindow(GetDlgItem(hwnd, ID_CANCEL), 480, 315, 110, 38, TRUE);
            g_editor_status = add_editor_control(hwnd, "STATIC",
                "Deixe nome e caminho vazios para remover um atalho.",
                SS_LEFT | SS_CENTERIMAGE, 24, 365, 560, 28, 0, instance);
            return 0;
        }
        case WM_COMMAND:
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
            SetBkColor(dc, RGB(14, 16, 20));
            return (LRESULT)g_background;
        }
        case WM_CTLCOLOREDIT: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(242, 244, 247));
            SetBkColor(dc, RGB(29, 33, 40));
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
    switch (message) {
        case WM_CREATE: {
            HINSTANCE instance = ((LPCREATESTRUCTA)lparam)->hInstance;
            add_button(hwnd, "VOLUME +", ID_VOL_UP, instance);
            add_button(hwnd, "VOLUME -", ID_VOL_DOWN, instance);
            add_button(hwnd, "MUTAR AUDIO", ID_MUTE, instance);
            add_button(hwnd, "PLAY / PAUSE", ID_MEDIA_PLAY, instance);
            add_button(hwnd, "PROXIMA FAIXA", ID_MEDIA_NEXT, instance);
            add_button(hwnd, "FAIXA ANTERIOR", ID_MEDIA_PREVIOUS, instance);
            for (int i = 0; i < DECK_APP_COUNT; ++i) {
                char label[DECK_TEXT_MAX + 24];
                if (g_config.apps[i].configured) {
                    snprintf(label, sizeof(label), "%s", g_config.apps[i].name);
                } else {
                    snprintf(label, sizeof(label), "CONFIGURAR APP %d", i + 1);
                }
                add_button(hwnd, label, ID_APP_1 + i, instance);
            }
            add_button(hwnd, "CONFIGURAR ATALHOS", ID_SETTINGS, instance);
            g_volume_label = CreateWindowA("STATIC", "VOLUME --", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                22, 22, 132, 28, hwnd, (HMENU)(INT_PTR)ID_VOLUME_LABEL, instance, NULL);
            g_volume_track = CreateWindowExA(0, TRACKBAR_CLASSA, "",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_NOTICKS | TBS_HORZ,
                154, 18, 500, 32, hwnd, (HMENU)(INT_PTR)ID_VOLUME_TRACK, instance, NULL);
            if (g_volume_track) {
                SendMessageA(g_volume_track, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
                SendMessageA(g_volume_track, TBM_SETPAGESIZE, 0, 5);
            }
            g_status = CreateWindowA(
                "STATIC", "Pronto. Escolha uma acao.",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                22, 0, 400, 34, hwnd, (HMENU)(INT_PTR)ID_STATUS, instance, NULL
            );
            if (g_status && g_status_font) SendMessageA(g_status, WM_SETFONT, (WPARAM)g_status_font, TRUE);
            layout_controls(hwnd);
            update_volume_ui();
            SetTimer(hwnd, ID_VOLUME_TIMER, 750, NULL);
            RegisterHotKey(hwnd, ID_HOTKEY_UP, MOD_CONTROL | MOD_ALT, VK_UP);
            RegisterHotKey(hwnd, ID_HOTKEY_DOWN, MOD_CONTROL | MOD_ALT, VK_DOWN);
            RegisterHotKey(hwnd, ID_HOTKEY_MUTE, MOD_CONTROL | MOD_ALT, 'M');
            return 0;
        }
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) {
                add_tray_icon(hwnd);
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            layout_controls(hwnd);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = 560;
            limits->ptMinTrackSize.y = 470;
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
        case WM_HSCROLL:
            if ((HWND)lparam == g_volume_track) {
                int position = (int)SendMessageA(g_volume_track, TBM_GETPOS, 0, 0);
                if (action_set_volume((float)position / 100.0f) == ACTION_SUCCESS) {
                    char message[64];
                    snprintf(message, sizeof(message), "Volume definido em %d%%.", position);
                    set_status(message);
                    update_volume_ui();
                } else {
                    set_status("Nao foi possivel alterar o volume.");
                }
            }
            return 0;
        case WM_TIMER:
            if (wparam == ID_VOLUME_TIMER) update_volume_ui();
            return 0;
        case WM_HOTKEY:
            if (wparam == ID_HOTKEY_UP) report_result(action_volume_step(0.05f), "Volume aumentado em 5%.");
            else if (wparam == ID_HOTKEY_DOWN) report_result(action_volume_step(-0.05f), "Volume reduzido em 5%.");
            else if (wparam == ID_HOTKEY_MUTE) report_result(action_toggle_mute(), "Estado do audio alternado.");
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
                return 0;
            }
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
                case ID_MEDIA_PLAY:
                    report_result(action_media_play_pause(), "Play/Pause enviado ao Windows.");
                    break;
                case ID_MEDIA_NEXT:
                    report_result(action_media_next(), "Comando de proxima faixa enviado.");
                    break;
                case ID_MEDIA_PREVIOUS:
                    report_result(action_media_previous(), "Comando de faixa anterior enviado.");
                    break;
                case ID_APP_1:
                case ID_APP_1 + 1:
                case ID_APP_1 + 2: {
                    int index = LOWORD(wparam) - ID_APP_1;
                    report_result(action_launch_app(&g_config.apps[index]), "Aplicativo iniciado.");
                    break;
                }
                case ID_SETTINGS:
                    open_editor(hwnd);
                    break;
            }
            return 0;
        case WM_CLOSE:
            add_tray_icon(hwnd);
            ShowWindow(hwnd, SW_HIDE);
            set_status("C-Deck continua ativo na bandeja do Windows.");
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, ID_VOLUME_TIMER);
            UnregisterHotKey(hwnd, ID_HOTKEY_UP);
            UnregisterHotKey(hwnd, ID_HOTKEY_DOWN);
            UnregisterHotKey(hwnd, ID_HOTKEY_MUTE);
            remove_tray_icon();
            if (g_button_font) DeleteObject(g_button_font);
            if (g_status_font) DeleteObject(g_status_font);
            if (g_background) DeleteObject(g_background);
            if (g_edit_background) DeleteObject(g_edit_background);
            g_button_font = NULL;
            g_status_font = NULL;
            g_background = NULL;
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
    g_button_font = CreateFontA(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_status_font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_background = CreateSolidBrush(RGB(14, 16, 20));
    g_edit_background = CreateSolidBrush(RGB(29, 33, 40));

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = g_background;
    window_class.lpszClassName = CLASS_NAME;
    if (!RegisterClassExA(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    ZeroMemory(&editor_class, sizeof(editor_class));
    editor_class.cbSize = sizeof(editor_class);
    editor_class.lpfnWndProc = editor_proc;
    editor_class.hInstance = instance;
    editor_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    editor_class.hbrBackground = g_background;
    editor_class.lpszClassName = EDITOR_CLASS;
    if (!RegisterClassExA(&editor_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 1;

    hwnd = CreateWindowExA(
        0, CLASS_NAME, "C-DECK | Painel de atalhos",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 540,
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
