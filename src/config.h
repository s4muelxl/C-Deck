#ifndef CDECK_CONFIG_H
#define CDECK_CONFIG_H
#include <stddef.h>

#define DECK_KEY_COUNT 15
#define DECK_APP_COUNT DECK_KEY_COUNT
#define DECK_CONFIG_VERSION 3
#define DECK_TEXT_MAX 260

typedef struct DeckApp {
    char name[DECK_TEXT_MAX];
    char path[DECK_TEXT_MAX];
    char icon_path[DECK_TEXT_MAX];
    int configured;
} DeckApp;

typedef struct DeckConfig {
    /* One persistent action definition for every visible key (5 x 3). */
    DeckApp apps[DECK_KEY_COUNT];
} DeckConfig;

int config_load(const char *path, DeckConfig *out_config);
int config_save(const char *path, const DeckConfig *config);
#endif
