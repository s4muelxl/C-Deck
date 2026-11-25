#include <windows.h>
#include <shellapi.h>
#include "apps.h"

/* O contrato de ShellExecuteW considera retorno maior que 32 como sucesso. */
BOOL cdeck_open_application(const wchar_t *path) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}
