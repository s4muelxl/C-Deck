#include "config.h"
#include "interface.h"
#include <windows.h>
#include <objbase.h>
#include <string.h>

static int build_config_path(char *out, DWORD capacity) {
    DWORD length = GetModuleFileNameA(NULL, out, capacity);
    char *separator;
    const char suffix[] = "config\\apps.cfg";

    if (length == 0 || length >= capacity) return 0;
    separator = strrchr(out, '\\');
    if (!separator) separator = strrchr(out, '/');
    if (!separator) return 0;
    separator[1] = '\\0';

    if (strlen(out) + sizeof(suffix) > capacity) return 0;
    strcat(out, suffix);
    return 1;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    DeckConfig config;
    char config_path[MAX_PATH];
    HRESULT com_result;
    int result;

    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        MessageBoxA(NULL, "Falha ao inicializar os servicos do Windows.", "C-Deck", MB_ICONERROR);
        return 1;
    }

    if (!build_config_path(config_path, MAX_PATH) || !config_load(config_path, &config)) {
        ZeroMemory(&config, sizeof(config));
        MessageBoxA(NULL, "Nao foi possivel ler config/apps.cfg ao lado do executavel. Os atalhos ficarao desativados.", "C-Deck", MB_ICONWARNING);
    }

    result = interface_run(instance, show_command, &config);
    if (SUCCEEDED(com_result)) CoUninitialize();
    return result;
}
