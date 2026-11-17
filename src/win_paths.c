#include "win_paths.h"
#include <windows.h>
#include <limits.h>
#include <string.h>
#include <wchar.h>

static wchar_t *read_module_path(size_t *length_out) {
    DWORD capacity = MAX_PATH;

    if (!length_out) return NULL;
    *length_out = 0;

    while (capacity <= DECK_PATH_WIDE_MAX) {
        wchar_t *buffer = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                               (SIZE_T)capacity * sizeof(wchar_t));
        DWORD length;

        if (!buffer) return NULL;
        SetLastError(ERROR_SUCCESS);
        length = GetModuleFileNameW(NULL, buffer, capacity);
        if (length == 0) {
            HeapFree(GetProcessHeap(), 0, buffer);
            return NULL;
        }
        if (length < capacity) {
            buffer[length] = L'\0';
            *length_out = (size_t)length;
            return buffer;
        }

        HeapFree(GetProcessHeap(), 0, buffer);
        if (capacity == DECK_PATH_WIDE_MAX) break;
        capacity = capacity > DECK_PATH_WIDE_MAX / 2
            ? DECK_PATH_WIDE_MAX
            : capacity * 2;
    }
    return NULL;
}

int deck_get_module_path_wide(wchar_t *out, size_t capacity) {
    wchar_t *module_path;
    size_t length = 0;

    if (!out || capacity == 0) return 0;
    out[0] = L'\0';
    module_path = read_module_path(&length);
    if (!module_path) return 0;
    if (length + 1 > capacity) {
        HeapFree(GetProcessHeap(), 0, module_path);
        return 0;
    }
    memcpy(out, module_path, (length + 1) * sizeof(wchar_t));
    HeapFree(GetProcessHeap(), 0, module_path);
    return 1;
}

static size_t bounded_wide_length(const wchar_t *value, size_t capacity) {
    size_t length = 0;
    if (!value) return capacity;
    while (length < capacity && value[length] != L'\0') ++length;
    return length;
}

int deck_config_path_from_module_path(const wchar_t *module_path, char *out, size_t capacity) {
    static const wchar_t suffix[] = L"config\\apps.cfg";
    static const wchar_t extended_prefix[] = L"\\\\?\\";
    static const wchar_t extended_unc_prefix[] = L"\\\\?\\UNC\\";
    wchar_t directory[DECK_PATH_WIDE_MAX];
    wchar_t extended[DECK_PATH_WIDE_MAX];
    wchar_t *separator;
    size_t length;
    size_t directory_length;
    size_t prefix_length;
    size_t tail_length;
    int required;

    if (!out || capacity == 0) return 0;
    out[0] = '\0';
    if (capacity > INT_MAX || !module_path) return 0;

    length = bounded_wide_length(module_path, DECK_PATH_WIDE_MAX);
    if (length == 0 || length >= DECK_PATH_WIDE_MAX) return 0;
    memcpy(directory, module_path, (length + 1) * sizeof(wchar_t));

    separator = wcsrchr(directory, L'\\');
    {
        wchar_t *forward_separator = wcsrchr(directory, L'/');
        if (!separator || (forward_separator && forward_separator > separator))
            separator = forward_separator;
    }
    if (!separator) return 0;
    separator[1] = L'\0';

    directory_length = bounded_wide_length(directory, DECK_PATH_WIDE_MAX);
    if (directory_length + (sizeof(suffix) / sizeof(suffix[0])) - 1 >= DECK_PATH_WIDE_MAX)
        return 0;
    wcscat(directory, suffix);
    for (wchar_t *cursor = directory; *cursor; ++cursor) {
        if (*cursor == L'/') *cursor = L'\\';
    }

    if (wcsncmp(directory, L"\\\\.\\", 4) == 0) return 0;
    if (wcsncmp(directory, extended_prefix, 4) == 0) {
        if (wcslen(directory) >= DECK_PATH_WIDE_MAX) return 0;
        wcscpy(extended, directory);
    } else if (directory[0] == L'\\' && directory[1] == L'\\') {
        prefix_length = sizeof(extended_unc_prefix) / sizeof(extended_unc_prefix[0]) - 1;
        tail_length = wcslen(directory + 2);
        if (prefix_length + tail_length >= DECK_PATH_WIDE_MAX) return 0;
        memcpy(extended, extended_unc_prefix, prefix_length * sizeof(wchar_t));
        memcpy(extended + prefix_length, directory + 2, (tail_length + 1) * sizeof(wchar_t));
    } else if (((directory[0] >= L'A' && directory[0] <= L'Z') ||
                (directory[0] >= L'a' && directory[0] <= L'z')) &&
               directory[1] == L':' && directory[2] == L'\\') {
        prefix_length = sizeof(extended_prefix) / sizeof(extended_prefix[0]) - 1;
        tail_length = wcslen(directory);
        if (prefix_length + tail_length >= DECK_PATH_WIDE_MAX) return 0;
        memcpy(extended, extended_prefix, prefix_length * sizeof(wchar_t));
        memcpy(extended + prefix_length, directory, (tail_length + 1) * sizeof(wchar_t));
    } else {
        wcscpy(extended, directory);
    }

    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, extended, -1,
                                   NULL, 0, NULL, NULL);
    if (required <= 0 || (size_t)required > capacity) return 0;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, extended, -1,
                             out, required, NULL, NULL)) {
        out[0] = '\0';
        return 0;
    }
    return 1;
}

int deck_get_config_path_utf8(char *out, size_t capacity) {
    wchar_t module_path[DECK_PATH_WIDE_MAX];
    if (!out || capacity == 0) return 0;
    out[0] = '\0';
    if (!deck_get_module_path_wide(module_path, DECK_PATH_WIDE_MAX)) return 0;
    return deck_config_path_from_module_path(module_path, out, capacity);
}
