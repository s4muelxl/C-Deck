#include <windows.h>
#include <objbase.h>
#include "interface.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command) {
    HRESULT com_result;
    int exit_code;

    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        MessageBoxW(NULL, L"Nao foi possivel iniciar os recursos do Windows.",
                    L"C-Deck", MB_OK | MB_ICONERROR);
        return 1;
    }

    exit_code = cdeck_run(instance, show_command);
    CoUninitialize();
    return exit_code;
}
