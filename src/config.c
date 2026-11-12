#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
            int number = 0;
            size_t length = strlen(entry);
            current = -1;
            if (length == 6 && _strnicmp(entry, "[app", 4) == 0 &&
                entry[4] >= '1' && entry[4] <= '3' && entry[5] == ']') {
                number = entry[4] - '0';
                current = number - 1;
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
        }
    }

    fclose(file);
    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        DeckApp *app = &out_config->apps[i];
        app->configured = app->name[0] != '\0' && app->path[0] != '\0';
    }
    return 1;
}
