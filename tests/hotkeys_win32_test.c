#include "../src/hotkeys_win32.h"

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#define TEST_WINDOW_CLASS L"CDeckHotkeyWin32AdapterTest"

static HWND create_test_window(HINSTANCE instance) {
    WNDCLASSEXW window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = DefWindowProcW;
    window_class.hInstance = instance;
    window_class.lpszClassName = TEST_WINDOW_CLASS;

    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return NULL;
    }

    return CreateWindowExW(0, TEST_WINDOW_CLASS, L"", WS_POPUP,
                           0, 0, 1, 1, NULL, NULL, instance, NULL);
}

int main(void) {
    const unsigned int first_id = 0x4D01u;
    const unsigned int second_id = 0x4D02u;
    const unsigned int modifiers = MOD_CONTROL | MOD_ALT | MOD_SHIFT;
    const unsigned int virtual_key = VK_F24;
    HINSTANCE instance = GetModuleHandleW(NULL);
    HWND first = create_test_window(instance);
    HWND second;
    unsigned long error_code = 0;

    assert(first != NULL);
    second = create_test_window(instance);
    assert(second != NULL);

    /* Reserve a deliberately uncommon chord, then verify the adapter reports
       the real Win32 conflict rather than silently treating it as success. */
    assert(RegisterHotKey(first, (int)first_id, modifiers, virtual_key));
    assert(!deck_hotkey_win32_register(second, second_id, modifiers,
                                       virtual_key, &error_code));
    assert(error_code == ERROR_HOTKEY_ALREADY_REGISTERED);

    assert(deck_hotkey_win32_unregister(first, first_id, &error_code));
    assert(error_code == ERROR_SUCCESS);

    /* Unregister is idempotent when Windows reports ERROR_HOTKEY_NOT_REGISTERED. */
    assert(deck_hotkey_win32_unregister(first, first_id, &error_code));
    assert(error_code == ERROR_SUCCESS);

    assert(deck_hotkey_win32_register(second, second_id, modifiers,
                                      virtual_key, &error_code));
    assert(error_code == ERROR_SUCCESS);
    assert(deck_hotkey_win32_unregister(second, second_id, &error_code));
    assert(error_code == ERROR_SUCCESS);

    assert(DestroyWindow(second));
    assert(DestroyWindow(first));
    assert(UnregisterClassW(TEST_WINDOW_CLASS, instance));

    puts("Win32 hotkey adapter conflict and idempotent cleanup: OK");
    return 0;
}
