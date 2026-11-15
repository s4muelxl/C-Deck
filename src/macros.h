#ifndef CDECK_MACROS_H
#define CDECK_MACROS_H

#include <stddef.h>

#define DECK_MACRO_COUNT 8
#define DECK_MACRO_STEP_COUNT 12
#define DECK_MACRO_NAME_MAX 64
#define DECK_MACRO_VALUE_MAX 260
#define DECK_MACRO_PATH_MAX 1040
#define DECK_MACRO_STEP_DELAY_MAX_MS 5000u
#define DECK_MACRO_TOTAL_DELAY_MAX_MS 10000u

typedef enum DeckMacroStepType {
    DECK_MACRO_STEP_ACTION = 1,
    DECK_MACRO_STEP_OPEN = 2,
    DECK_MACRO_STEP_DELAY = 3
} DeckMacroStepType;

typedef struct DeckMacroStep {
    DeckMacroStepType type;
    char value[DECK_MACRO_VALUE_MAX];
    unsigned int delay_ms;
} DeckMacroStep;

typedef struct DeckMacro {
    char name[DECK_MACRO_NAME_MAX];
    unsigned int step_count;
    DeckMacroStep steps[DECK_MACRO_STEP_COUNT];
    int configured;
} DeckMacro;

typedef struct DeckMacroStore {
    DeckMacro macros[DECK_MACRO_COUNT];
    char path[DECK_MACRO_PATH_MAX];
    int loaded;
    int read_only;
} DeckMacroStore;

typedef enum DeckMacroResult {
    DECK_MACRO_OK = 0,
    DECK_MACRO_INVALID_ARGUMENT,
    DECK_MACRO_NOT_FOUND,
    DECK_MACRO_INVALID_DEFINITION,
    DECK_MACRO_EXECUTION_FAILED,
    DECK_MACRO_CANCELLED,
    DECK_MACRO_STORAGE_ERROR
} DeckMacroResult;

/* Executors return zero on success. OPEN receives one path/URL, never a command line. */
typedef int (*DeckMacroActionExecutor)(const char *action_id, void *context);
typedef int (*DeckMacroOpenExecutor)(const char *target, void *context);
typedef int (*DeckMacroDelayExecutor)(unsigned int milliseconds, void *context);
typedef int (*DeckMacroCancelCheck)(void *context);

int macro_step_parse(const char *text, DeckMacroStep *step);
int macro_validate(const DeckMacro *macro);
int macro_step_insert(DeckMacro *macro, unsigned int index, const DeckMacroStep *step);
int macro_step_replace(DeckMacro *macro, unsigned int index, const DeckMacroStep *step);
int macro_step_remove(DeckMacro *macro, unsigned int index);
int macro_step_move(DeckMacro *macro, unsigned int from_index, unsigned int to_index);
int macro_store_load(DeckMacroStore *store, const char *config_path);
int macro_store_save(DeckMacroStore *store);
size_t macro_store_count(const DeckMacroStore *store);
const DeckMacro *macro_store_get(const DeckMacroStore *store, unsigned int macro_id);
DeckMacroResult macro_execute(const DeckMacro *macro,
                              DeckMacroActionExecutor action_executor,
                              DeckMacroOpenExecutor open_executor,
                              DeckMacroDelayExecutor delay_executor,
                              DeckMacroCancelCheck cancel_check,
                              void *context);

#endif
