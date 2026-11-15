#include "../src/profiles.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

static void write_text(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    fputs(contents, file);
    assert(fclose(file) == 0);
}

static int file_contains(const char *path, const char *needle) {
    char line[512];
    FILE *file = fopen(path, "r");
    int found = 0;
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, needle)) {
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}

static void remove_utf8_profile_files(const char *prefix) {
    wchar_t wide_path[DECK_PROFILE_PATH_MAX];
    char path[256];
    const char *suffixes[] = {
        "-apps.cfg", "-apps.cfg.bak",
        "-apps-streaming.cfg", "-apps-streaming.cfg.bak",
        "-apps-productivity.cfg", "-apps-productivity.cfg.bak",
        "-apps-gaming.cfg", "-apps-gaming.cfg.bak",
        "-apps-preferences.cfg", "-apps-preferences.cfg.bak"
    };
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
        snprintf(path, sizeof(path), "%s%s", prefix, suffixes[i]);
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                                wide_path, DECK_PROFILE_PATH_MAX) > 0) DeleteFileW(wide_path);
        snprintf(path, sizeof(path), "%s%s.tmp", prefix, suffixes[i]);
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                                wide_path, DECK_PROFILE_PATH_MAX) > 0) DeleteFileW(wide_path);
    }
}

static void remove_profile_files(const char *prefix) {
    char path[256];
    const char *suffixes[] = {
        "-apps.cfg", "-apps.cfg.bak",
        "-apps-streaming.cfg", "-apps-streaming.cfg.bak",
        "-apps-productivity.cfg", "-apps-productivity.cfg.bak",
        "-apps-gaming.cfg", "-apps-gaming.cfg.bak",
        "-apps-preferences.cfg", "-apps-preferences.cfg.bak"
    };
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
        snprintf(path, sizeof(path), "%s%s", prefix, suffixes[i]);
        remove(path);
        snprintf(path, sizeof(path), "%s%s.tmp", prefix, suffixes[i]);
        remove(path);
    }
}

int main(void) {
    DeckConfig initial;
    DeckConfig active;
    DeckConfig reloaded_active;
    DeckProfileStore store;
    DeckProfileStore reloaded;
    DeckPreferences next;

    remove_profile_files("test-profile");
    config_set_defaults(&initial);
    strcpy(initial.apps[6].name, "Atalho padrão");
    strcpy(initial.apps[6].path, "default-app.exe");
    initial.apps[6].configured = 1;

    assert(profiles_load(&store, "test-profile-apps.cfg", &initial, &active) == 1);
    assert(store.preferences.active_profile == DECK_PROFILE_DEFAULT);
    assert(active.apps[6].configured == 1);
    assert(strcmp(active.apps[6].name, "Atalho padrão") == 0);
    assert(wcscmp(profile_name(DECK_PROFILE_STREAMING), L"Streaming / OBS") == 0);
    assert(profile_id_from_name(L"Produtividade") == DECK_PROFILE_PRODUCTIVITY);
    assert(profile_id_from_name(L"perfil inexistente") == -1);

    assert(strcmp(store.profiles[DECK_PROFILE_STREAMING].apps[6].path, "obs64.exe") == 0);
    assert(strcmp(store.profiles[DECK_PROFILE_PRODUCTIVITY].apps[7].path, "calc.exe") == 0);
    assert(strcmp(store.profiles[DECK_PROFILE_GAMING].apps[8].path, "spotify.exe") == 0);

    next = store.preferences;
    next.active_profile = DECK_PROFILE_STREAMING;
    next.minimize_to_tray = 0;
    next.enable_hotkeys = 0;
    next.window_opacity = 85;
    assert(profiles_apply_preferences(&store, &next, &active, &active) == 1);
    assert(store.preferences.active_profile == DECK_PROFILE_STREAMING);
    assert(active.apps[6].configured == 1);
    assert(strcmp(active.apps[6].name, "OBS Studio") == 0);
    assert(store.profile_persisted[DECK_PROFILE_STREAMING] == 1);

    strcpy(active.apps[6].name, "OBS personalizado");
    strcpy(active.apps[6].path, "C:\\Apps\\obs.exe");
    assert(profiles_save_active(&store, &active) == 1);

    next = store.preferences;
    next.window_opacity = 90;
    assert(profiles_save_preferences(&store, &next) == 1);
    assert(file_contains("test-profile-apps-preferences.cfg", "window_opacity=90"));
    assert(file_contains("test-profile-apps-preferences.cfg.bak", "window_opacity=85"));
    assert(file_contains("test-profile-apps-streaming.cfg", "name=OBS personalizado"));

    config_set_defaults(&initial);
    assert(profiles_load(&reloaded, "test-profile-apps.cfg", &initial, &reloaded_active) == 1);
    assert(reloaded.preferences.active_profile == DECK_PROFILE_STREAMING);
    assert(reloaded.preferences.minimize_to_tray == 0);
    assert(reloaded.preferences.enable_hotkeys == 0);
    assert(reloaded.preferences.window_opacity == 90);
    assert(strcmp(reloaded_active.apps[6].name, "OBS personalizado") == 0);
    assert(strcmp(reloaded_active.apps[6].path, "C:\\Apps\\obs.exe") == 0);

    next = reloaded.preferences;
    next.active_profile = DECK_PROFILE_DEFAULT;
    assert(profiles_apply_preferences(&reloaded, &next, &reloaded_active, &reloaded_active) == 1);
    assert(strcmp(reloaded_active.apps[6].name, "Atalho padrão") == 0);
    assert(strcmp(reloaded_active.apps[6].path, "default-app.exe") == 0);

    write_text("test-profile-apps-preferences.cfg",
               "[meta]\nversion=99\n[preferences]\nactive_profile=3\n"
               "minimize_to_tray=1\nenable_hotkeys=1\nwindow_opacity=100\n");
    assert(profiles_load(&store, "test-profile-apps.cfg", &initial, &active) == 1);
    assert(store.preferences_recovered == 1);
    assert(store.preferences.active_profile == DECK_PROFILE_DEFAULT);
    assert(file_contains("test-profile-apps-preferences.cfg", "version=99"));

    remove_profile_files("test-profile");

    /* A future-version default profile must remain untouched on load failure. */
    remove_profile_files("test-future");
    write_text("test-future-apps.cfg", "[meta]\nversion=99\n[app1]\nname=Future\npath=future.exe\n");
    assert(profiles_load(&store, "test-future-apps.cfg", &initial, &active) == 0);
    assert(file_contains("test-future-apps.cfg", "version=99"));
    remove_profile_files("test-future");

    /* UTF-8 paths are converted to UTF-16 for profile and preferences files. */
    remove_utf8_profile_files("perfil-ação");
    assert(profiles_load(&store, "perfil-ação-apps.cfg", &initial, &active) == 1);
    next = store.preferences;
    next.active_profile = DECK_PROFILE_GAMING;
    assert(profiles_apply_preferences(&store, &next, &active, &active) == 1);
    assert(profiles_load(&reloaded, "perfil-ação-apps.cfg", &initial, &reloaded_active) == 1);
    assert(reloaded.preferences.active_profile == DECK_PROFILE_GAMING);
    assert(strcmp(reloaded_active.apps[8].name, "Spotify") == 0);
    remove_utf8_profile_files("perfil-ação");

    puts("profiles/preferences persistence: OK");
    return 0;
}
