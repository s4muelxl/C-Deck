#include "config.h"
#include "interface.h"
#include <windows.h>
#include <objbase.h>
#include <string.h>
#include <wchar.h>

static int build_config_path(char *out, DWORD capacity) {
    wchar_t module_path[MAX_PATH];
    wchar_t *separator;
    const wchar_t suffix[] = L"config\\apps.cfg";
    DWORD length = GetModuleFileNameW(NULL, module_path, MAX_PATH);

    if (length == 0 || length >= MAX_PATH) return 0;
    separator = wcsrchr(module_path, L'\\');
    if (!separator) separator = wcsrchr(module_path, L'/');
    if (!separator) return 0;
    separator[1] = L'\0';
    if (wcslen(module_path) + (sizeof(suffix) / sizeof(suffix[0])) > MAX_PATH) return 0;
    wcscat(module_path, suffix);

    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, module_path, -1,
                               out, (int)capacity, NULL, NULL) > 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    DeckConfig config;
    char config_path[MAX_PATH * 4];
    HRESULT com_result;
    int result;

    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        MessageBoxW(NULL, L"Falha ao inicializar os serviços do Windows.", L"C-Deck", MB_ICONERROR);
        return 1;
    }

    if (!build_config_path(config_path, (DWORD)sizeof(config_path)) || !config_load(config_path, &config)) {
        config_set_defaults(&config);
        MessageBoxW(NULL, L"Não foi possível ler config/apps.cfg ao lado do executável. Os controles internos padrão continuam disponíveis; os demais atalhos ficam desativados.", L"C-Deck", MB_ICONWARNING);
    }

    result = interface_run(instance, show_command, &config);
    if (SUCCEEDED(com_result)) CoUninitialize();
    return result;
}
