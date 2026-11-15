#include "profiles.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <wchar.h>

static const wchar_t *PROFILE_NAMES[DECK_PROFILE_COUNT] = {
    L"Default Profile",
    L"Streaming / OBS",
    L"Produtividade",
    L"Jogos / Gaming"
};

static const char *PROFILE_SUFFIXES[DECK_PROFILE_COUNT] = {
    "",
    "streaming",
    "productivity",
    "gaming"
};

static char *trim_text(char *text) {
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    if (!*text) return text;
    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) *end-- = '\0';
    return text;
}

static void copy_text(char *destination, size_t capacity, const char *source) {
    if (!capacity) return;
    strncpy(destination, source, capacity - 1);
    destination[capacity - 1] = '\0';
}

static int utf8_to_wide_path(const char *path, wchar_t *wide, size_t capacity) {
    if (!path || !wide || capacity == 0 || capacity > INT_MAX) return 0;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                               wide, (int)capacity) > 0;
}

/* Returns -1 for an invalid UTF-8 path, 0 when missing and 1 when present. */
static int path_exists(const char *path) {
    wchar_t wide[DECK_PROFILE_PATH_MAX];
    if (!utf8_to_wide_path(path, wide, sizeof(wide) / sizeof(wide[0]))) return -1;
    return GetFileAttributesW(wide) == INVALID_FILE_ATTRIBUTES ? 0 : 1;
}

static int build_store_paths(DeckProfileStore *store, const char *config_path) {
    const char *slash_a;
    const char *slash_b;
    const char *last_slash;
    const char *filename;
    const char *extension;
    size_t directory_length;
    size_t stem_length;

    if (!store || !config_path || !config_path[0]) return 0;
    slash_a = strrchr(config_path, '/');
    slash_b = strrchr(config_path, '\\');
    last_slash = slash_a;
    if (!last_slash || (slash_b && slash_b > last_slash)) last_slash = slash_b;
    directory_length = last_slash ? (size_t)(last_slash - config_path + 1) : 0;
    filename = config_path + directory_length;
    extension = strrchr(filename, '.');
    stem_length = extension ? (size_t)(extension - filename) : strlen(filename);
    if (stem_length == 0 || directory_length > INT_MAX || stem_length > INT_MAX) return 0;

    if (snprintf(store->profile_paths[DECK_PROFILE_DEFAULT],
                 sizeof(store->profile_paths[DECK_PROFILE_DEFAULT]), "%s", config_path) < 0) return 0;

    for (int i = 1; i < DECK_PROFILE_COUNT; ++i) {
        int written = snprintf(store->profile_paths[i], sizeof(store->profile_paths[i]),
                               "%.*s%.*s-%s.cfg",
                               (int)directory_length, config_path,
                               (int)stem_length, filename, PROFILE_SUFFIXES[i]);
        if (written < 0 || (size_t)written >= sizeof(store->profile_paths[i])) return 0;
    }

    {
        int written = snprintf(store->preferences_path, sizeof(store->preferences_path),
                               "%.*s%.*s-preferences.cfg",
                               (int)directory_length, config_path,
                               (int)stem_length, filename);
        if (written < 0 || (size_t)written >= sizeof(store->preferences_path)) return 0;
    }
    return 1;
}

const wchar_t *profile_name(int profile_id) {
    if (profile_id < 0 || profile_id >= DECK_PROFILE_COUNT) return PROFILE_NAMES[DECK_PROFILE_DEFAULT];
    return PROFILE_NAMES[profile_id];
}

int profile_id_from_name(const wchar_t *name) {
    if (!name) return -1;
    for (int i = 0; i < DECK_PROFILE_COUNT; ++i) {
        if (wcscmp(name, PROFILE_NAMES[i]) == 0) return i;
    }
    return -1;
}

void profiles_set_default_preferences(DeckPreferences *preferences) {
    if (!preferences) return;
    preferences->active_profile = DECK_PROFILE_DEFAULT;
    preferences->minimize_to_tray = 1;
    preferences->enable_hotkeys = 1;
    preferences->window_opacity = 100;
}

static int validate_preferences(const DeckPreferences *preferences) {
    if (!preferences) return 0;
    return preferences->active_profile >= 0 && preferences->active_profile < DECK_PROFILE_COUNT &&
           (preferences->minimize_to_tray == 0 || preferences->minimize_to_tray == 1) &&
           (preferences->enable_hotkeys == 0 || preferences->enable_hotkeys == 1) &&
           preferences->window_opacity >= 60 && preferences->window_opacity <= 100;
}

static int parse_integer(const char *value, int *out_value) {
    char *end = NULL;
    long parsed;
    if (!value || !out_value) return 0;
    parsed = strtol(value, &end, 10);
    if (end == value || *trim_text(end) != '\0' || parsed < INT_MIN || parsed > INT_MAX) return 0;
    *out_value = (int)parsed;
    return 1;
}

static int load_preferences_file(const char *path, DeckPreferences *preferences) {
    wchar_t wide_path[DECK_PROFILE_PATH_MAX];
    FILE *file;
    char line[256];
    int section = 0;
    int version = 0;
    int version_seen = 0;
    int sections_seen = 0;
    unsigned int seen = 0;

    if (!utf8_to_wide_path(path, wide_path, sizeof(wide_path) / sizeof(wide_path[0]))) return 0;
    file = _wfopen(wide_path, L"r");
    if (!file) return 0;

    while (fgets(line, sizeof(line), file)) {
        char *entry;
        char *equals;
        int parsed;

        if (!strchr(line, '\n') &&
            (strlen(line) == sizeof(line) - 1 || !feof(file))) {
            fclose(file);
            return 0;
        }
        entry = trim_text(line);
        if (!entry[0] || entry[0] == '#' || entry[0] == ';') continue;
        if (entry[0] == '[') {
            int next_section = (_stricmp(entry, "[meta]") == 0) ? 1 :
                               (_stricmp(entry, "[preferences]") == 0) ? 2 : 0;
            int section_bit = next_section == 1 ? 1 : next_section == 2 ? 2 : 0;
            if (section_bit && (sections_seen & section_bit)) {
                fclose(file);
                return 0;
            }
            sections_seen |= section_bit;
            section = next_section;
            continue;
        }
        equals = strchr(entry, '=');
        if (!equals) continue;
        *equals++ = '\0';
        entry = trim_text(entry);
        equals = trim_text(equals);

        if (section == 1 && _stricmp(entry, "version") == 0) {
            if (version_seen) {
                fclose(file);
                return 0;
            }
            version_seen = 1;
            if (!parse_integer(equals, &version)) {
                fclose(file);
                return 0;
            }
        } else if (section == 2) {
            if (!parse_integer(equals, &parsed)) {
                fclose(file);
                return 0;
            }
            if (_stricmp(entry, "active_profile") == 0) {
                preferences->active_profile = parsed;
                seen |= 1u;
            } else if (_stricmp(entry, "minimize_to_tray") == 0) {
                preferences->minimize_to_tray = parsed;
                seen |= 2u;
            } else if (_stricmp(entry, "enable_hotkeys") == 0) {
                preferences->enable_hotkeys = parsed;
                seen |= 4u;
            } else if (_stricmp(entry, "window_opacity") == 0) {
                preferences->window_opacity = parsed;
                seen |= 8u;
            }
        }
    }

    if (ferror(file)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    return sections_seen == 3 && version_seen && version == 1 &&
           seen == 15u && validate_preferences(preferences);
}

static int save_preferences_file(const char *path, const DeckPreferences *preferences) {
    wchar_t wide_path[DECK_PROFILE_PATH_MAX];
    wchar_t temporary[DECK_PROFILE_PATH_MAX];
    wchar_t backup[DECK_PROFILE_PATH_MAX];
    FILE *file;
    size_t length;
    int written = 1;

    if (!validate_preferences(preferences) ||
        !utf8_to_wide_path(path, wide_path, sizeof(wide_path) / sizeof(wide_path[0]))) return 0;
    length = wcslen(wide_path);
    if (length + 5 >= sizeof(temporary) / sizeof(temporary[0])) return 0;
    wcscpy(temporary, wide_path);
    wcscat(temporary, L".tmp");
    wcscpy(backup, wide_path);
    wcscat(backup, L".bak");

    file = _wfopen(temporary, L"w");
    if (!file) return 0;
    if (fprintf(file,
                "; C-Deck preferences. Schema versioned independently from action slots.\n"
                "[meta]\nversion=1\n\n[preferences]\n"
                "active_profile=%d\nminimize_to_tray=%d\nenable_hotkeys=%d\nwindow_opacity=%d\n",
                preferences->active_profile, preferences->minimize_to_tray,
                preferences->enable_hotkeys, preferences->window_opacity) < 0) written = 0;
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

static void apply_profile_preset(DeckConfig *config, int profile_id) {
    static const char *names[DECK_PROFILE_COUNT][3] = {
        { "", "", "" },
        { "OBS Studio", "Discord", "Twitch" },
        { "Bloco de Notas", "Calculadora", "Paint" },
        { "Steam", "Discord", "Spotify" }
    };
    static const char *paths[DECK_PROFILE_COUNT][3] = {
        { "", "", "" },
        { "obs64.exe", "Discord.exe", "https://dashboard.twitch.tv" },
        { "notepad.exe", "calc.exe", "mspaint.exe" },
        { "steam.exe", "Discord.exe", "spotify.exe" }
    };
    static const char *icons[DECK_PROFILE_COUNT][3] = {
        { "", "", "" },
        { "icon:9", "icon:8", "icon:11" },
        { "icon:1", "icon:2", "icon:3" },
        { "icon:10", "icon:8", "icon:7" }
    };

    if (!config || profile_id <= DECK_PROFILE_DEFAULT || profile_id >= DECK_PROFILE_COUNT) return;
    for (int i = 0; i < 3; ++i) {
        DeckApp *app = &config->apps[6 + i];
        copy_text(app->name, sizeof(app->name), names[profile_id][i]);
        copy_text(app->path, sizeof(app->path), paths[profile_id][i]);
        copy_text(app->icon_path, sizeof(app->icon_path), icons[profile_id][i]);
        app->configured = 1;
    }
}

int profiles_load(DeckProfileStore *store, const char *config_path,
                  const DeckConfig *fallback_config, DeckConfig *active_config) {
    int exists;

    if (!store || !fallback_config || !active_config ||
        !build_store_paths(store, config_path)) return 0;
    ZeroMemory(store->profiles, sizeof(store->profiles));
    ZeroMemory(store->profile_persisted, sizeof(store->profile_persisted));
    store->preferences_recovered = 0;
    store->storage_warning = 0;
    store->profiles[DECK_PROFILE_DEFAULT] = *fallback_config;

    exists = path_exists(store->profile_paths[DECK_PROFILE_DEFAULT]);
    if (exists < 0) return 0;
    if (exists && !config_load(store->profile_paths[DECK_PROFILE_DEFAULT],
                               &store->profiles[DECK_PROFILE_DEFAULT])) return 0;
    store->profile_persisted[DECK_PROFILE_DEFAULT] = exists;

    for (int i = 1; i < DECK_PROFILE_COUNT; ++i) {
        exists = path_exists(store->profile_paths[i]);
        if (exists < 0) return 0;
        if (exists) {
            if (!config_load(store->profile_paths[i], &store->profiles[i])) return 0;
            store->profile_persisted[i] = 1;
        } else {
            store->profiles[i] = store->profiles[DECK_PROFILE_DEFAULT];
            apply_profile_preset(&store->profiles[i], i);
        }
    }

    profiles_set_default_preferences(&store->preferences);
    exists = path_exists(store->preferences_path);
    if (exists < 0) return 0;
    if (exists && !load_preferences_file(store->preferences_path, &store->preferences)) {
        profiles_set_default_preferences(&store->preferences);
        store->preferences_recovered = 1;
    }

    if (!store->profile_persisted[store->preferences.active_profile]) {
        int active_id = store->preferences.active_profile;
        if (!config_save(store->profile_paths[active_id], &store->profiles[active_id])) {
            store->preferences.active_profile = DECK_PROFILE_DEFAULT;
            store->storage_warning = 1;
        } else {
            store->profile_persisted[active_id] = 1;
        }
    }

    *active_config = store->profiles[store->preferences.active_profile];
    return 1;
}

int profiles_save_active(DeckProfileStore *store, const DeckConfig *config) {
    int active_id;
    if (!store || !config || !validate_preferences(&store->preferences)) return 0;
    active_id = store->preferences.active_profile;
    if (!config_save(store->profile_paths[active_id], config)) return 0;
    store->profiles[active_id] = *config;
    store->profile_persisted[active_id] = 1;
    return 1;
}

int profiles_save_preferences(DeckProfileStore *store, const DeckPreferences *preferences) {
    if (!store || !preferences || !save_preferences_file(store->preferences_path, preferences)) return 0;
    store->preferences = *preferences;
    return 1;
}

int profiles_apply_preferences(DeckProfileStore *store, const DeckPreferences *preferences,
                               const DeckConfig *current_config, DeckConfig *active_config) {
    int target_id;
    if (!store || !validate_preferences(preferences) || !current_config || !active_config) return 0;
    if (!profiles_save_active(store, current_config)) return 0;

    target_id = preferences->active_profile;
    if (!store->profile_persisted[target_id]) {
        if (!config_save(store->profile_paths[target_id], &store->profiles[target_id])) return 0;
        store->profile_persisted[target_id] = 1;
    }
    if (!profiles_save_preferences(store, preferences)) return 0;
    *active_config = store->profiles[target_id];
    return 1;
}

int profiles_activate(DeckProfileStore *store, int profile_id,
                      const DeckConfig *current_config, DeckConfig *active_config) {
    DeckPreferences next;
    if (!store || profile_id < 0 || profile_id >= DECK_PROFILE_COUNT) return 0;
    next = store->preferences;
    next.active_profile = profile_id;
    return profiles_apply_preferences(store, &next, current_config, active_config);
}
