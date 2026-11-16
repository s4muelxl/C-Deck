#ifndef CDECK_CONFIG_H
#define CDECK_CONFIG_H
#include <stddef.h>

#define DECK_KEY_COUNT 15
#define DECK_APP_COUNT DECK_KEY_COUNT
#define DECK_CONFIG_VERSION 4
#define DECK_TEXT_MAX 260
#define DECK_PATH_TEXT_MAX 8192
#define DECK_CONFIG_LINE_MAX (DECK_PATH_TEXT_MAX + 16)

/* Windows extended paths are bounded to 32,767 UTF-16 code units. */
#define DECK_PATH_WIDE_MAX 32768
#define DECK_PATH_UTF8_MAX (DECK_PATH_WIDE_MAX * 4 + 1)

typedef struct DeckApp {
    char name[DECK_TEXT_MAX];
    char path[DECK_PATH_TEXT_MAX];
    char icon_path[DECK_PATH_TEXT_MAX];
    int configured;
} DeckApp;

typedef struct DeckConfig {
    /* One persistent action definition for every visible key (5 x 3). */
    DeckApp apps[DECK_KEY_COUNT];
} DeckConfig;

void config_set_defaults(DeckConfig *config);
int config_load(const char *path, DeckConfig *out_config);
int config_save(const char *path, const DeckConfig *config);
#endif
