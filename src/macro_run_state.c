#include "macro_run_state.h"

static uint32_t next_generation(uint32_t generation) {
    ++generation;
    return generation == 0 ? 1u : generation;
}

void deck_macro_run_state_init(DeckMacroRunState *state) {
    if (!state) return;
    state->generation = 0;
    state->active = 0;
}

uint32_t deck_macro_run_state_begin(DeckMacroRunState *state) {
    if (!state) return 0;
    state->generation = next_generation(state->generation);
    state->active = 1;
    return state->generation;
}

int deck_macro_run_state_complete(DeckMacroRunState *state, uint32_t generation) {
    if (!state || generation == 0 || !state->active ||
        state->generation != generation) return 0;
    state->active = 0;
    return 1;
}

void deck_macro_run_state_invalidate(DeckMacroRunState *state) {
    if (!state) return;
    state->generation = next_generation(state->generation);
    state->active = 0;
}

int deck_macro_run_state_is_active(const DeckMacroRunState *state) {
    return state && state->active;
}
