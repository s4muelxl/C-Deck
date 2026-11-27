#include <windows.h>
#include <shellapi.h>
#include "apps.h"

/* O contrato de ShellExecuteW considera retorno maior que 32 como sucesso. */
BOOL cdeck_open_application(const wchar_t *path) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}

/* Abre a sobreposicao nativa de captura (equivalente a Win + Shift + S). */
BOOL cdeck_open_screen_capture(void) {
    INPUT keys[6] = {0};
    keys[0].type = INPUT_KEYBOARD; keys[0].ki.wVk = VK_LWIN;
    keys[1].type = INPUT_KEYBOARD; keys[1].ki.wVk = VK_LSHIFT;
    keys[2].type = INPUT_KEYBOARD; keys[2].ki.wVk = 'S';
    keys[3].type = INPUT_KEYBOARD; keys[3].ki.wVk = 'S'; keys[3].ki.dwFlags = KEYEVENTF_KEYUP;
    keys[4].type = INPUT_KEYBOARD; keys[4].ki.wVk = VK_LSHIFT; keys[4].ki.dwFlags = KEYEVENTF_KEYUP;
    keys[5].type = INPUT_KEYBOARD; keys[5].ki.wVk = VK_LWIN; keys[5].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(6, keys, sizeof(INPUT)) == 6;
}
