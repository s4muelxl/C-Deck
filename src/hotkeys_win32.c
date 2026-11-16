#include "hotkeys_win32.h"

#include <windows.h>

int deck_hotkey_win32_register(void *context,
                               unsigned int id,
                               unsigned int modifiers,
                               unsigned int virtual_key,
                               unsigned long *error_code) {
    SetLastError(ERROR_SUCCESS);
    if (RegisterHotKey((HWND)context, (int)id, modifiers, virtual_key)) {
        if (error_code) *error_code = ERROR_SUCCESS;
        return 1;
    }

    if (error_code) *error_code = (unsigned long)GetLastError();
    return 0;
}

int deck_hotkey_win32_unregister(void *context,
                                 unsigned int id,
                                 unsigned long *error_code) {
    DWORD last_error;

    SetLastError(ERROR_SUCCESS);
    if (UnregisterHotKey((HWND)context, (int)id)) {
        if (error_code) *error_code = ERROR_SUCCESS;
        return 1;
    }

    last_error = GetLastError();
    if (last_error == ERROR_HOTKEY_NOT_REGISTERED) {
        /* The requested end state is already true: this ID is not registered. */
        if (error_code) *error_code = ERROR_SUCCESS;
        return 1;
    }

    if (error_code) *error_code = (unsigned long)last_error;
    return 0;
}
