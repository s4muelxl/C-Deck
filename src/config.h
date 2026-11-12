#ifndef CDECK_CONFIG_H
#define CDECK_CONFIG_H
#include <stddef.h>
#define DECK_APP_COUNT 3
#define DECK_TEXT_MAX 260
typedef struct DeckApp { char name[DECK_TEXT_MAX]; char path[DECK_TEXT_MAX]; char icon_path[DECK_TEXT_MAX]; int configured; } DeckApp;
typedef struct DeckConfig { DeckApp apps[DECK_APP_COUNT]; } DeckConfig;
int config_load(const char *path, DeckConfig *out_config);
int config_save(const char *path, const DeckConfig *config);
#endif
