#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
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

int config_load(const char *path, DeckConfig *out_config) {
    FILE *file;
    char line[1024];
    int current = -1;

    if (!path || !out_config) return 0;
    memset(out_config, 0, sizeof(*out_config));
    file = fopen(path, "r");
    if (!file) return 0;

    while (fgets(line, sizeof(line), file)) {
        char *entry = trim(line);
        char *equals;

        if (*entry == '\0' || *entry == '#' || *entry == ';') continue;
        if (*entry == '[') {
            current = -1;
            if (strlen(entry) == 6 && _strnicmp(entry, "[app", 4) == 0 &&
                entry[4] >= '1' && entry[4] <= '3' && entry[5] == ']') {
                current = entry[4] - '1';
            }
            continue;
        }

        if (current < 0 || !(equals = strchr(entry, '='))) continue;
        *equals++ = '\0';
        entry = trim(entry);
        equals = trim(equals);

        if (_stricmp(entry, "name") == 0) {
            copy_value(out_config->apps[current].name, DECK_TEXT_MAX, equals);
        } else if (_stricmp(entry, "path") == 0) {
            copy_value(out_config->apps[current].path, DECK_TEXT_MAX, equals);
        } else if (_stricmp(entry, "icon") == 0) {
            copy_value(out_config->apps[current].icon_path, DECK_TEXT_MAX, equals);
        }
    }

    fclose(file);
    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        DeckApp *app = &out_config->apps[i];
        app->configured = app->name[0] != '\0' && app->path[0] != '\0';
    }
    return 1;
}

int config_save(const char *path, const DeckConfig *config) {
    char temporary[MAX_PATH];
    FILE *file;
    int written = 1;

    if (!path || !config || strlen(path) + 5 >= sizeof(temporary)) return 0;
    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        const DeckApp *app = &config->apps[i];
        int has_name = app->name[0] != '\0';
        int has_path = app->path[0] != '\0';
        if (has_name != has_path) return 0;
        if (strpbrk(app->name, "\r\n") || strpbrk(app->path, "\r\n") || strpbrk(app->icon_path, "\r\n")) return 0;
    }

    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    file = fopen(temporary, "w");
    if (!file) return 0;

    if (fprintf(file, "; Arquivo de atalhos do C-DECK. Pode ser editado manualmente.\n") < 0) written = 0;
    for (int i = 0; written && i < DECK_APP_COUNT; ++i) {
        const DeckApp *app = &config->apps[i];
        if (fprintf(file, "\n[app%d]\nname=%s\npath=%s\nicon=%s\n",
                    i + 1, app->name, app->path, app->icon_path) < 0) written = 0;
    }
    if (fclose(file) != 0) written = 0;
    if (!written) {
        remove(temporary);
        return 0;
    }
    if (!MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        remove(temporary);
        return 0;
    }
    return 1;
}
