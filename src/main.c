#include "config.h"
#include "interface.h"
#include "win_paths.h"
#include <windows.h>
#include <objbase.h>
#include <string.h>
#include <wchar.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    DeckConfig config;
    char config_path[DECK_PATH_UTF8_MAX];
    HRESULT com_result;
    int result;

    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        MessageBoxW(NULL, L"Falha ao inicializar os serviços do Windows.", L"C-Deck", MB_ICONERROR);
        return 1;
    }

    if (!deck_get_config_path_utf8(config_path, sizeof(config_path)) || !config_load(config_path, &config)) {
        config_set_defaults(&config);
        MessageBoxW(NULL, L"Não foi possível ler config/apps.cfg ao lado do executável. Os controles internos padrão continuam disponíveis; os demais atalhos ficam desativados.", L"C-Deck", MB_ICONWARNING);
    }

    result = interface_run(instance, show_command, &config);
    if (SUCCEEDED(com_result)) CoUninitialize();
    return result;
}
