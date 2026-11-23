#include <windows.h>
#include "interface.h"
#include "audio.h"
#include "apps.h"

#define ID_VOLUME_UP 101
#define ID_VOLUME_DOWN 102
#define ID_MUTE 103
#define ID_NOTEPAD 104
#define ID_CALCULATOR 105
#define ID_PAINT 106

#define BUTTON_WIDTH 155
#define BUTTON_HEIGHT 42
#define LEFT_COLUMN 18
#define RIGHT_COLUMN 187
#define FIRST_ROW 82
#define ROW_GAP 50

static HWND status_label;

static void set_status(const wchar_t *message) {
    SetWindowTextW(status_label, message);
}

static void add_button(HWND window, HINSTANCE instance,
                       const wchar_t *label, int id, int x, int y) {
    HWND button = CreateWindowW(
        L"BUTTON", label, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, BUTTON_WIDTH, BUTTON_HEIGHT,
        window, (HMENU)(INT_PTR)id, instance, NULL
    );
    if (button != NULL) {
        SendMessageW(button, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
}

static void create_controls(HWND window, HINSTANCE instance) {
    HWND title = CreateWindowW(
        L"STATIC", L"C-DECK", WS_CHILD | WS_VISIBLE,
        18, 16, 320, 26, window, NULL, instance, NULL
    );
    HWND subtitle = CreateWindowW(
        L"STATIC", L"Atalhos rápidos do Windows", WS_CHILD | WS_VISIBLE,
        18, 43, 320, 20, window, NULL, instance, NULL
    );

    if (title != NULL) SendMessageW(title, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    if (subtitle != NULL) SendMessageW(subtitle, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    add_button(window, instance, L"Volume +", ID_VOLUME_UP, LEFT_COLUMN, FIRST_ROW);
    add_button(window, instance, L"Volume -", ID_VOLUME_DOWN, RIGHT_COLUMN, FIRST_ROW);
    add_button(window, instance, L"Mudo", ID_MUTE, LEFT_COLUMN, FIRST_ROW + ROW_GAP);
    add_button(window, instance, L"Bloco de Notas", ID_NOTEPAD, RIGHT_COLUMN, FIRST_ROW + ROW_GAP);
    add_button(window, instance, L"Calculadora", ID_CALCULATOR, LEFT_COLUMN, FIRST_ROW + 2 * ROW_GAP);
    add_button(window, instance, L"Paint", ID_PAINT, RIGHT_COLUMN, FIRST_ROW + 2 * ROW_GAP);

    status_label = CreateWindowW(
        L"STATIC", L"Pronto para usar.", WS_CHILD | WS_VISIBLE,
        18, 245, 320, 22, window, NULL, instance, NULL
    );
    if (status_label != NULL) {
        SendMessageW(status_label, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
}

static LRESULT CALLBACK window_procedure(HWND window, UINT message,
                                          WPARAM wparam, LPARAM lparam) {
    (void)lparam;

    switch (message) {
        case WM_CREATE:
            create_controls(window, ((LPCREATESTRUCTW)lparam)->hInstance);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case ID_VOLUME_UP:
                    set_status(cdeck_change_volume(0.05f) ? L"Volume aumentado." : L"Nao foi possivel alterar o volume.");
                    return 0;
                case ID_VOLUME_DOWN:
                    set_status(cdeck_change_volume(-0.05f) ? L"Volume reduzido." : L"Nao foi possivel alterar o volume.");
                    return 0;
                case ID_MUTE:
                    set_status(cdeck_toggle_mute() ? L"Estado do audio alterado." : L"Nao foi possivel alterar o audio.");
                    return 0;
                case ID_NOTEPAD:
                    set_status(cdeck_open_application(L"notepad.exe") ? L"Bloco de Notas aberto." : L"Nao foi possivel abrir o Bloco de Notas.");
                    return 0;
                case ID_CALCULATOR:
                    set_status(cdeck_open_application(L"calc.exe") ? L"Calculadora aberta." : L"Nao foi possivel abrir a Calculadora.");
                    return 0;
                case ID_PAINT:
                    set_status(cdeck_open_application(L"mspaint.exe") ? L"Paint aberto." : L"Nao foi possivel abrir o Paint.");
                    return 0;
                default:
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

int cdeck_run(HINSTANCE instance, int show_command) {
    WNDCLASSW window_class = {0};
    HWND window;
    MSG message;

    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"CDeckMainWindow";
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&window_class)) return 1;

    window = CreateWindowW(
        L"CDeckMainWindow", L"C-Deck",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 330,
        NULL, NULL, instance, NULL
    );
    if (window == NULL) return 1;

    ShowWindow(window, show_command);
    UpdateWindow(window);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}
