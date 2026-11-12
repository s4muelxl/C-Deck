#include "config.h"
#include "interface.h"
#include <windows.h>
#include <objbase.h>
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command_line,int show_command) {
    DeckConfig config; HRESULT com_result;
    (void)previous; (void)command_line;
    com_result=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    if (FAILED(com_result) && com_result!=RPC_E_CHANGED_MODE) { MessageBoxA(NULL,"Falha ao inicializar os servicos do Windows.","C-Deck",MB_ICONERROR); return 1; }
    if (!config_load("config/apps.cfg",&config)) { ZeroMemory(&config,sizeof(config)); MessageBoxA(NULL,"Nao foi possivel ler config/apps.cfg. Os atalhos ficarao desativados.","C-Deck",MB_ICONWARNING); }
    int result=interface_run(instance,show_command,&config);
    if (SUCCEEDED(com_result)) CoUninitialize();
    return result;
}
