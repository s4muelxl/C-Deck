#define COBJMACROS
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <initguid.h>
#define WINDOW_CLASS L"CDeckMainWindow"
#define ID_VOLUME_UP 101
#define ID_VOLUME_DOWN 102
#define ID_MUTE 103
#define ID_APP_1 104
#define ID_APP_2 105
#define ID_APP_3 106
static HWND g_status;

static IAudioEndpointVolume *get_audio_volume(void) {
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *volume = NULL;
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                                &IID_IMMDeviceEnumerator, (void **)&enumerator))) return NULL;
    if (SUCCEEDED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eMultimedia, &device))) {
        IMMDevice_Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void **)&volume);
    }
    if (device) IMMDevice_Release(device);
    IMMDeviceEnumerator_Release(enumerator);
    return volume;
}
static void show_status(const wchar_t *message) { SetWindowTextW(g_status, message); }

static void change_volume(float amount) {
    IAudioEndpointVolume *audio = get_audio_volume();
    float current = 0.0f;
    if (!audio) { show_status(L"Dispositivo de áudio não encontrado."); return; }
    if (SUCCEEDED(IAudioEndpointVolume_GetMasterVolumeLevelScalar(audio, &current))) {
        current += amount;
        if (current < 0.0f) current = 0.0f;
        if (current > 1.0f) current = 1.0f;
        if (SUCCEEDED(IAudioEndpointVolume_SetMasterVolumeLevelScalar(audio, current, NULL))) {
            wchar_t message[64];
            wsprintfW(message, L"Volume: %d%%", (int)(current * 100));
            show_status(message);
        } else show_status(L"Não foi possível alterar o volume.");
    }
    IAudioEndpointVolume_Release(audio);
}
static void toggle_mute(void) {
    IAudioEndpointVolume *audio = get_audio_volume();
    BOOL muted = FALSE;
    if (!audio) { show_status(L"Dispositivo de áudio não encontrado."); return; }
    if (SUCCEEDED(IAudioEndpointVolume_GetMute(audio, &muted)) &&
        SUCCEEDED(IAudioEndpointVolume_SetMute(audio, !muted, NULL)))
        show_status(muted ? L"Som ativado." : L"Som mutado.");
    else show_status(L"Não foi possível alterar o mudo.");
    IAudioEndpointVolume_Release(audio);
}
static void open_app(const wchar_t *program) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", program, NULL, NULL, SW_SHOWNORMAL);
    show_status((INT_PTR)result > 32 ? L"Aplicativo aberto." : L"Não foi possível abrir o aplicativo.");
}
static void add_button(HWND window, const wchar_t *label, int id, int x, int y) {
    CreateWindowW(L"BUTTON", label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, 145, 55, window, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
}
static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    (void)lparam;
    switch (message) {
        case WM_CREATE:
            add_button(window, L"Volume +", ID_VOLUME_UP, 25, 25);
            add_button(window, L"Volume -", ID_VOLUME_DOWN, 190, 25);
            add_button(window, L"Mute", ID_MUTE, 355, 25);
            add_button(window, L"Bloco de Notas", ID_APP_1, 25, 105);
            add_button(window, L"Calculadora", ID_APP_2, 190, 105);
            add_button(window, L"Paint", ID_APP_3, 355, 105);
            g_status = CreateWindowW(L"STATIC", L"Pronto para usar.", WS_CHILD | WS_VISIBLE | SS_CENTER,
                25, 185, 475, 30, window, NULL, GetModuleHandleW(NULL), NULL);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case ID_VOLUME_UP: change_volume(0.05f); break;
                case ID_VOLUME_DOWN: change_volume(-0.05f); break;
                case ID_MUTE: toggle_mute(); break;
                case ID_APP_1: open_app(L"notepad.exe"); break;
                case ID_APP_2: open_app(L"calc.exe"); break;
                case ID_APP_3: open_app(L"mspaint.exe"); break;
            }
            return 0;
        case WM_CLOSE: DestroyWindow(window); return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
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
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        MessageBoxW(NULL, L"Não foi possível iniciar os recursos de áudio.", L"C-Deck", MB_ICONERROR);
        return 1;
    }
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = WINDOW_CLASS;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&window_class)) {
        MessageBoxW(NULL, L"Não foi possível criar a janela.", L"C-Deck", MB_ICONERROR);
        if (SUCCEEDED(com_result)) CoUninitialize();
        return 1;
    }
    window = CreateWindowW(WINDOW_CLASS, L"C-Deck", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 545, 270, NULL, NULL, instance, NULL);
    if (!window) { if (SUCCEEDED(com_result)) CoUninitialize(); return 1; }
    ShowWindow(window, show_command);
    UpdateWindow(window);
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (SUCCEEDED(com_result)) CoUninitialize();
    return (int)message.wParam;
}
