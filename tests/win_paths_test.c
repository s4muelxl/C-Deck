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


static wchar_t long_directory[DECK_PATH_WIDE_MAX];
static wchar_t extended_root[DECK_PATH_WIDE_MAX];
static wchar_t config_directory[DECK_PATH_WIDE_MAX];
static wchar_t fake_module[DECK_PATH_WIDE_MAX];
static wchar_t config_path_wide[DECK_PATH_WIDE_MAX];
static wchar_t backup_path_wide[DECK_PATH_WIDE_MAX];

static int make_extended_absolute(const wchar_t *source, wchar_t *out, size_t capacity) {
    static const wchar_t drive_prefix[] = L"\\\\?\\";
    static const wchar_t unc_prefix[] = L"\\\\?\\UNC\\";
    size_t length;
    if (!source || !out || capacity == 0) return 0;
    out[0] = L'\0';
    if (wcsncmp(source, L"\\\\?\\", 4) == 0) {
        length = wcslen(source);
        if (length + 1 > capacity) return 0;
        wcscpy(out, source);
        return 1;
    }
    if (source[0] == L'\\' && source[1] == L'\\') {
        length = wcslen(source + 2);
        if (8 + length + 1 > capacity) return 0;
        wcscpy(out, unc_prefix);
        wcscat(out, source + 2);
        return 1;
    }
    if (((source[0] >= L'A' && source[0] <= L'Z') ||
         (source[0] >= L'a' && source[0] <= L'z')) &&
        source[1] == L':' && source[2] == L'\\') {
        length = wcslen(source);
        if (4 + length + 1 > capacity) return 0;
        wcscpy(out, drive_prefix);
        wcscat(out, source);
        return 1;
    }
    return 0;
}

static void test_long_path_config_io(void) {
    wchar_t temp_path[DECK_PATH_WIDE_MAX];
    wchar_t temp_name[MAX_PATH];
    wchar_t cleanup[DECK_PATH_WIDE_MAX];
    wchar_t *last_separator;
    DWORD temp_length;
    size_t root_length;
    DeckConfig config;
    DeckConfig loaded;

    temp_length = GetTempPathW(DECK_PATH_WIDE_MAX, temp_path);
    assert(temp_length > 0 && temp_length < DECK_PATH_WIDE_MAX);
    assert(temp_length < MAX_PATH);
    assert(GetTempFileNameW(temp_path, L"cdk", 0, temp_name) != 0);
    assert(DeleteFileW(temp_name));
    assert(make_extended_absolute(temp_name, extended_root, DECK_PATH_WIDE_MAX));
    assert(CreateDirectoryW(extended_root, NULL));

    wcscpy(long_directory, extended_root);
    root_length = wcslen(extended_root);
    for (int i = 0; i < 24; ++i) {
        const wchar_t segment[] = L"segment0123456789";
        assert(wcslen(long_directory) + wcslen(segment) + 2 < DECK_PATH_WIDE_MAX);
        wcscat(long_directory, L"\\");
        wcscat(long_directory, segment);
        assert(CreateDirectoryW(long_directory, NULL));
    }
    assert(wcslen(long_directory) > MAX_PATH);

    wcscpy(config_directory, long_directory);
    wcscat(config_directory, L"\\config");
    assert(CreateDirectoryW(config_directory, NULL));

    wcscpy(fake_module, long_directory);
    wcscat(fake_module, L"\\C-Deck.exe");
    assert(deck_config_path_from_module_path(fake_module, actual_utf8, sizeof(actual_utf8)));
    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, actual_utf8, -1,
                               config_path_wide, DECK_PATH_WIDE_MAX) > 0);
    wcscpy(backup_path_wide, config_path_wide);
    wcscat(backup_path_wide, L".bak");

    config_set_defaults(&config);
    strcpy(config.apps[14].name, "Café long path");
    strcpy(config.apps[14].path, "action:volume_up");
    assert(config_save(actual_utf8, &config));
    assert(config_save(actual_utf8, &config));
    assert(GetFileAttributesW(config_path_wide) != INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesW(backup_path_wide) != INVALID_FILE_ATTRIBUTES);
    assert(config_load(actual_utf8, &loaded));
    assert(strcmp(loaded.apps[14].name, "Café long path") == 0);

    assert(DeleteFileW(config_path_wide));
    assert(DeleteFileW(backup_path_wide));
    assert(RemoveDirectoryW(config_directory));

    wcscpy(cleanup, long_directory);
    for (int i = 0; i < 24; ++i) {
        assert(RemoveDirectoryW(cleanup));
        last_separator = wcsrchr(cleanup, L'\\');
        assert(last_separator != NULL && (size_t)(last_separator - cleanup) >= root_length);
        *last_separator = L'\0';
    }
    assert(RemoveDirectoryW(extended_root));
}

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

    test_long_path_config_io();
    puts("Win32 dynamic module path, long-path config persistence and Unicode paths: OK");
    return 0;
}
