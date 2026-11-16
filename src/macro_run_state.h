#ifndef CDECK_MACRO_RUN_STATE_H
#define CDECK_MACRO_RUN_STATE_H
#include <stdint.h>

/* UI-thread-owned token state for rejecting stale worker completion messages. */
typedef struct DeckMacroRunState {
    uint32_t generation;
    int active;
} DeckMacroRunState;

void deck_macro_run_state_init(DeckMacroRunState *state);
uint32_t deck_macro_run_state_begin(DeckMacroRunState *state);
int deck_macro_run_state_complete(DeckMacroRunState *state, uint32_t generation);
void deck_macro_run_state_invalidate(DeckMacroRunState *state);
int deck_macro_run_state_is_active(const DeckMacroRunState *state);

#endif
