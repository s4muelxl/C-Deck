#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

static char *trim(char *text) {
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    if (!*text) return text;
    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) *end-- = '\0';
    return text;
}

static void copy_value(char *destination, size_t capacity, const char *source) {
    if (!capacity) return;
    strncpy(destination, source, capacity - 1);
    destination[capacity - 1] = '\0';
}

static void set_default_action(DeckApp *app, const char *name, const char *path) {
    copy_value(app->name, sizeof(app->name), name);
    copy_value(app->path, sizeof(app->path), path);
    app->configured = 1;
}

static void set_builtin_actions(DeckConfig *config) {
    static const char *names[6] = {
        "Volume +", "Mudo Som", "Play / Pause",
        "Anterior", "Próximo", "Volume -"
    };
    static const char *paths[6] = {
        "action:volume_up", "action:audio_mute", "action:media_play_pause",
        "action:media_previous", "action:media_next", "action:volume_down"
    };
    for (int i = 0; i < 6; ++i) {
        if (!config->apps[i].name[0] && !config->apps[i].path[0]) {
            set_default_action(&config->apps[i], names[i], paths[i]);
        }
    }
}

static int utf8_path_to_wide(const char *path, wchar_t *wide, size_t capacity) {
    int converted;
    if (!path || !wide || capacity == 0 || capacity > INT_MAX) return 0;
    converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, (int)capacity);
    return converted > 0;
}

int config_load(const char *path, DeckConfig *out_config) {
    FILE *file;
    char line[1024];
    DeckApp parsed_apps[DECK_KEY_COUNT];
    int current = -1;
    int has_legacy_apps = 0;
    int has_extended_apps = 0;
    int in_meta = 0;
    int schema_version = 1;
    wchar_t wide_path[MAX_PATH * 4];

    if (!path || !out_config || !utf8_path_to_wide(path, wide_path, sizeof(wide_path) / sizeof(wide_path[0]))) return 0;
    memset(out_config, 0, sizeof(*out_config));
    memset(parsed_apps, 0, sizeof(parsed_apps));
    file = _wfopen(wide_path, L"r");
    if (!file) return 0;

    while (fgets(line, sizeof(line), file)) {
        char *entry = trim(line);
        char *equals;

        if (*entry == '\0' || *entry == '#' || *entry == ';') continue;
        if (*entry == '[') {
            char *section_end = NULL;
            long app_number = -1;
            current = -1;
            in_meta = (_stricmp(entry, "[meta]") == 0);
            if (!in_meta && _strnicmp(entry, "[app", 4) == 0) {
                app_number = strtol(entry + 4, &section_end, 10);
                if (section_end != entry + 4 && section_end && section_end[0] == ']' &&
                    section_end[1] == '\0' && app_number >= 1 && app_number <= DECK_KEY_COUNT) {
                    current = (int)app_number - 1;
                    if (current < 3) has_legacy_apps = 1;
                    else has_extended_apps = 1;
                }
            }
            continue;
        }

        if (!(equals = strchr(entry, '='))) continue;
        *equals++ = '\0';
        entry = trim(entry);
        equals = trim(equals);

        if (in_meta && _stricmp(entry, "version") == 0) {
            char *end = NULL;
            long parsed = strtol(equals, &end, 10);
            if (end == equals || *trim(end) != '\0' || parsed < 1 || parsed > INT_MAX) {
                schema_version = 0;
            } else {
                schema_version = (int)parsed;
            }
            continue;
        }
        if (current < 0) continue;

        if (_stricmp(entry, "name") == 0) {
            copy_value(parsed_apps[current].name, DECK_TEXT_MAX, equals);
        } else if (_stricmp(entry, "path") == 0) {
            copy_value(parsed_apps[current].path, DECK_TEXT_MAX, equals);
        } else if (_stricmp(entry, "icon") == 0) {
            copy_value(parsed_apps[current].icon_path, DECK_TEXT_MAX, equals);
        }
    }

    fclose(file);
    if (schema_version == 0 || schema_version > DECK_CONFIG_VERSION) return 0;

    if (schema_version >= 3) {
        memcpy(out_config->apps, parsed_apps, sizeof(parsed_apps));
    } else {
        /*
         * v1/v2 stored the nine configurable destinations separately from six
         * hard-coded system/media keys. In v3 every visual key owns one slot:
         * old app1..app9 become new app7..app15 without changing their order.
         */
        int legacy_count = schema_version == 2 ? 9 : (has_extended_apps ? 9 : 3);
        if (has_legacy_apps || has_extended_apps) {
            for (int i = 0; i < legacy_count; ++i) {
                out_config->apps[i + 6] = parsed_apps[i];
            }
        }
        if (schema_version == 1 && has_legacy_apps && !has_extended_apps) {
            static const char *legacy_names[6] = {
                "Mudo Mic", "Trocar Áudio", "Navegador",
                "Tarefas", "Captura", "Configurações"
            };
            static const char *legacy_paths[6] = {
                "action:mic_mute", "action:sound_devices", "https://www.google.com",
                "taskmgr.exe", "SnippingTool.exe", "ms-settings:"
            };
            for (int i = 0; i < 6; ++i) {
                set_default_action(&out_config->apps[i + 9], legacy_names[i], legacy_paths[i]);
            }
        }
        if (has_legacy_apps || has_extended_apps) set_builtin_actions(out_config);
    }

    for (int i = 0; i < DECK_KEY_COUNT; ++i) {
        DeckApp *app = &out_config->apps[i];
        app->configured = app->name[0] != '\0' && app->path[0] != '\0';
    }
    return 1;
}

int config_save(const char *path, const DeckConfig *config) {
    wchar_t wide_path[MAX_PATH * 4];
    wchar_t temporary[MAX_PATH * 4];
    wchar_t backup[MAX_PATH * 4];
    FILE *file;
    int written = 1;
    size_t path_length;

    if (!path || !config ||
        !utf8_path_to_wide(path, wide_path, sizeof(wide_path) / sizeof(wide_path[0]))) return 0;
    path_length = wcslen(wide_path);
    if (path_length + 5 >= sizeof(temporary) / sizeof(temporary[0])) return 0;
    wcscpy(temporary, wide_path);
    wcscat(temporary, L".tmp");
    wcscpy(backup, wide_path);
    wcscat(backup, L".bak");
    for (int i = 0; i < DECK_KEY_COUNT; ++i) {
        const DeckApp *app = &config->apps[i];
        int has_name = app->name[0] != '\0';
        int has_path = app->path[0] != '\0';
        if (has_name != has_path) return 0;
        if (strpbrk(app->name, "\r\n") || strpbrk(app->path, "\r\n") || strpbrk(app->icon_path, "\r\n")) return 0;
    }

    file = _wfopen(temporary, L"w");
    if (!file) return 0;

    if (fprintf(file, "; Arquivo de ações do C-DECK. Pode ser editado manualmente.\n[meta]\nversion=%d\n", DECK_CONFIG_VERSION) < 0) written = 0;
    for (int i = 0; written && i < DECK_KEY_COUNT; ++i) {
        const DeckApp *app = &config->apps[i];
        if (fprintf(file, "\n[app%d]\nname=%s\npath=%s\nicon=%s\n",
                    i + 1, app->name, app->path, app->icon_path) < 0) written = 0;
    }
    if (fclose(file) != 0) written = 0;
    if (!written) {
        DeleteFileW(temporary);
        return 0;
    }

    if (GetFileAttributesW(wide_path) != INVALID_FILE_ATTRIBUTES &&
        !CopyFileW(wide_path, backup, FALSE)) {
        DeleteFileW(temporary);
        return 0;
    }
    if (!MoveFileExW(temporary, wide_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary);
        return 0;
    }
    return 1;
}
