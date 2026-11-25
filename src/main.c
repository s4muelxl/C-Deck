#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include "interface.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command) {
    HRESULT com_result;
    BOOL com_initialized = FALSE;
    char log_buffer[256];
    int remote_session;
    int exit_code;

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    cdeck_log("INICIO", "=== Inicializacao do C-Deck (WinMain) ===");
    remote_session = GetSystemMetrics(SM_REMOTESESSION);
    snprintf(log_buffer, sizeof(log_buffer), "Sessao Windows: remote_session=%d", remote_session);
    cdeck_log("AMBIENTE", log_buffer);

    /* O modulo de audio utiliza interfaces COM da Windows Core Audio API (STA). */
    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(com_result)) {
        com_initialized = TRUE;
        snprintf(log_buffer, sizeof(log_buffer), "COM inicializado (HRESULT=0x%08lX)", (unsigned long)com_result);
        cdeck_log("COM", log_buffer);
    } else if (com_result == RPC_E_CHANGED_MODE) {
        /* Thread ja configurada em outro modelo COM; CoUninitialize nao deve ser chamado. */
        snprintf(log_buffer, sizeof(log_buffer),
                 "COM ja inicializado em outro modo (HRESULT=0x%08lX); prosseguindo",
                 (unsigned long)com_result);
        cdeck_log("COM", log_buffer);
    } else {
        wchar_t dialog_text[256];
        snprintf(log_buffer, sizeof(log_buffer), "Falha em CoInitializeEx (HRESULT=0x%08lX)", (unsigned long)com_result);
        cdeck_log("ERRO", log_buffer);
        swprintf(dialog_text, sizeof(dialog_text) / sizeof(dialog_text[0]),
                 L"Nao foi possivel iniciar os recursos COM do Windows (HRESULT 0x%08lX).\r\n"
                 L"Consulte o arquivo C-Deck-startup.log na pasta temporaria do Windows.",
                 (unsigned long)com_result);
        MessageBoxW(NULL, dialog_text, L"C-Deck - Falha na inicializacao COM", MB_OK | MB_ICONERROR);
        return 1;
    }

    cdeck_log("INICIO", "Chamando cdeck_run()");
    exit_code = cdeck_run();
    snprintf(log_buffer, sizeof(log_buffer), "cdeck_run finalizado com codigo %d", exit_code);
    cdeck_log(exit_code == 0 ? "SAIDA" : "ERRO", log_buffer);

    if (com_initialized) {
        CoUninitialize();
    }
    return exit_code;
}
