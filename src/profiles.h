#ifndef CDECK_PROFILES_H
#define CDECK_PROFILES_H

#include "config.h"
#include <wchar.h>

#define DECK_PROFILE_COUNT 4
#define DECK_PROFILE_PATH_MAX 1040

typedef enum DeckProfileId {
    DECK_PROFILE_DEFAULT = 0,
    DECK_PROFILE_STREAMING = 1,
    DECK_PROFILE_PRODUCTIVITY = 2,
    DECK_PROFILE_GAMING = 3
} DeckProfileId;

typedef struct DeckPreferences {
    int active_profile;
    int minimize_to_tray;
    int enable_hotkeys;
    int window_opacity;
} DeckPreferences;

typedef struct DeckProfileStore {
    DeckConfig profiles[DECK_PROFILE_COUNT];
    DeckPreferences preferences;
    char profile_paths[DECK_PROFILE_COUNT][DECK_PROFILE_PATH_MAX];
    char preferences_path[DECK_PROFILE_PATH_MAX];
    int profile_persisted[DECK_PROFILE_COUNT];
    int preferences_recovered;
} DeckProfileStore;

const wchar_t *profile_name(int profile_id);
int profile_id_from_name(const wchar_t *name);
void profiles_set_default_preferences(DeckPreferences *preferences);
int profiles_load(DeckProfileStore *store, const char *config_path,
                  const DeckConfig *fallback_config, DeckConfig *active_config);
int profiles_save_active(DeckProfileStore *store, const DeckConfig *config);
int profiles_save_preferences(DeckProfileStore *store, const DeckPreferences *preferences);
int profiles_apply_preferences(DeckProfileStore *store, const DeckPreferences *preferences,
                               const DeckConfig *current_config, DeckConfig *active_config);
int profiles_activate(DeckProfileStore *store, int profile_id,
                      const DeckConfig *current_config, DeckConfig *active_config);

#endif
