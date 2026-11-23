#define COBJMACROS
#include <windows.h>
#include <shellapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <objbase.h>

#define ID_VOLUME_UP 101
#define ID_VOLUME_DOWN 102
#define ID_MUTE 103
#define ID_NOTEPAD 104
#define ID_CALCULATOR 105
#define ID_PAINT 106

static HWND status_label;

static IAudioEndpointVolume *get_audio_volume(void) {
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *volume = NULL;
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                                &IID_IMMDeviceEnumerator, (void **)&enumerator))) return NULL;
    if (FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eMultimedia, &device))) {
        IMMDeviceEnumerator_Release(enumerator);
        return NULL;
    }
    if (FAILED(IMMDevice_Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void **)&volume))) volume = NULL;
    IMMDevice_Release(device);
    IMMDeviceEnumerator_Release(enumerator);
    return volume;
}

static void show_status(const wchar_t *message) {
    SetWindowTextW(status_label, message);
}

static void change_volume(float difference) {
    IAudioEndpointVolume *volume = get_audio_volume();
    float current = 0.0f;
    if (!volume) { show_status(L"Dispositivo de áudio indisponível."); return; }
    if (SUCCEEDED(IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume, &current))) {
        current += difference;
        if (current < 0.0f) current = 0.0f;
        if (current > 1.0f) current = 1.0f;
        show_status(SUCCEEDED(IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume, current, NULL))
            ? L"Volume atualizado." : L"Falha ao alterar o volume.");
    } else show_status(L"Falha ao consultar o volume.");
    IAudioEndpointVolume_Release(volume);
}

static void toggle_mute(void) {
    IAudioEndpointVolume *volume = get_audio_volume();
    BOOL muted = FALSE;
    if (!volume) { show_status(L"Dispositivo de áudio indisponível."); return; }
    if (SUCCEEDED(IAudioEndpointVolume_GetMute(volume, &muted)) &&
        SUCCEEDED(IAudioEndpointVolume_SetMute(volume, !muted, NULL)))
        show_status(muted ? L"Som ativado." : L"Som mutado.");
    else show_status(L"Falha ao alterar o áudio.");
    IAudioEndpointVolume_Release(volume);
}

static void open_application(const wchar_t *path) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
    show_status((INT_PTR)result > 32 ? L"Aplicativo aberto." : L"Falha ao abrir aplicativo.");
}

static void add_button(HWND window, const wchar_t *text, int id, int x, int y) {
    CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, 145, 48, window, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
}

static LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    (void)lparam;
    switch (message) {
        case WM_CREATE:
            CreateWindowW(L"STATIC", L"C-DECK", WS_CHILD | WS_VISIBLE,
                25, 18, 450, 30, window, NULL, GetModuleHandleW(NULL), NULL);
            CreateWindowW(L"STATIC", L"Controles rápidos do computador", WS_CHILD | WS_VISIBLE,
                25, 50, 450, 22, window, NULL, GetModuleHandleW(NULL), NULL);
            add_button(window, L"Volume +", ID_VOLUME_UP, 25, 95);
            add_button(window, L"Volume -", ID_VOLUME_DOWN, 185, 95);
            add_button(window, L"Mudo", ID_MUTE, 345, 95);
            add_button(window, L"Bloco de Notas", ID_NOTEPAD, 25, 155);
            add_button(window, L"Calculadora", ID_CALCULATOR, 185, 155);
            add_button(window, L"Paint", ID_PAINT, 345, 155);
            status_label = CreateWindowW(L"STATIC", L"Pronto para usar.", WS_CHILD | WS_VISIBLE,
                25, 225, 465, 25, window, NULL, GetModuleHandleW(NULL), NULL);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case ID_VOLUME_UP: change_volume(0.05f); return 0;
                case ID_VOLUME_DOWN: change_volume(-0.05f); return 0;
                case ID_MUTE: toggle_mute(); return 0;
                case ID_NOTEPAD: open_application(L"notepad.exe"); return 0;
                case ID_CALCULATOR: open_application(L"calc.exe"); return 0;
                case ID_PAINT: open_application(L"mspaint.exe"); return 0;
                default: break;
            }
            break;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: break;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    WNDCLASSW window_class = {0};
    HWND window;
    MSG message;
    HRESULT com_result;
    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        MessageBoxW(NULL, L"Falha ao iniciar os recursos do Windows.", L"C-Deck", MB_OK | MB_ICONERROR);
        return 1;
    }

    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"CDeckMainWindow";
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&window_class)) { CoUninitialize(); return 1; }

    window = CreateWindowW(L"CDeckMainWindow", L"C-Deck",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 535, 315, NULL, NULL, instance, NULL);
    if (!window) { CoUninitialize(); return 1; }

    ShowWindow(window, show_command);
    UpdateWindow(window);
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    CoUninitialize();
    return (int)message.wParam;
}