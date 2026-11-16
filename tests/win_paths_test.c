#include "win_paths.h"
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static char actual_utf8[DECK_PATH_UTF8_MAX];
static wchar_t actual_wide[DECK_PATH_WIDE_MAX];
static wchar_t long_module[DECK_PATH_WIDE_MAX];
static wchar_t expected[DECK_PATH_WIDE_MAX];

static void assert_path(const wchar_t *module_path, const wchar_t *expected_path) {
    assert(deck_config_path_from_module_path(module_path, actual_utf8, sizeof(actual_utf8)));
    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, actual_utf8, -1,
                               actual_wide, DECK_PATH_WIDE_MAX) > 0);
    assert(wcscmp(actual_wide, expected_path) == 0);
}

int main(void) {
    wchar_t module_path[DECK_PATH_WIDE_MAX];
    wchar_t short_buffer[8];
    wchar_t *separator;
    size_t used;

    assert_path(L"C:\\Users\\José\\音楽\\C-Deck.exe",
                L"\\\\?\\C:\\Users\\José\\音楽\\config\\apps.cfg");
    assert_path(L"\\\\server\\share\\Apps\\C-Deck.exe",
                L"\\\\?\\UNC\\server\\share\\Apps\\config\\apps.cfg");
    assert_path(L"\\\\?\\C:\\Apps\\C-Deck.exe",
                L"\\\\?\\C:\\Apps\\config\\apps.cfg");

    wcscpy(long_module, L"C:\\");
    used = wcslen(long_module);
    for (int i = 0; i < 24; ++i) {
        const wchar_t segment[] = L"segment0123456789\\";
        size_t count = sizeof(segment) / sizeof(segment[0]);
        assert(used + count < DECK_PATH_WIDE_MAX);
        memcpy(long_module + used, segment, count * sizeof(wchar_t));
        used += count - 1;
    }
    wcscpy(long_module + used, L"C-Deck.exe");
    assert(wcslen(long_module) > MAX_PATH);

    wcscpy(expected, L"\\\\?\\");
    wcscat(expected, long_module);
    separator = wcsrchr(expected, L'\\');
    assert(separator != NULL);
    separator[1] = L'\0';
    wcscat(expected, L"config\\apps.cfg");
    assert_path(long_module, expected);

    assert(!deck_config_path_from_module_path(L"C-Deck.exe", actual_utf8, sizeof(actual_utf8)));
    assert(actual_utf8[0] == '\0');
    assert(!deck_config_path_from_module_path(L"C:\\Apps\\C-Deck.exe",
                                               actual_utf8, sizeof(short_buffer)));
    assert(actual_utf8[0] == '\0');

    assert(deck_get_module_path_wide(module_path, DECK_PATH_WIDE_MAX));
    assert(module_path[0] != L'\0');
    assert(!deck_get_module_path_wide(short_buffer, sizeof(short_buffer) / sizeof(short_buffer[0])));
    assert(short_buffer[0] == L'\0');
    assert(deck_get_config_path_utf8(actual_utf8, sizeof(actual_utf8)));
    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, actual_utf8, -1,
                               actual_wide, DECK_PATH_WIDE_MAX) > 0);
    assert(wcsstr(actual_wide, L"config\\apps.cfg") != NULL);

    puts("Win32 dynamic module path and extended config path: OK");
    return 0;
}
