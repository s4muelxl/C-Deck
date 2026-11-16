#include "macro_run_state.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    DeckMacroRunState state = {0, 0};
    uint32_t first, second, third;

    deck_macro_run_state_init(&state);
    assert(!deck_macro_run_state_is_active(&state));
    first = deck_macro_run_state_begin(&state);
    assert(first != 0 && deck_macro_run_state_is_active(&state));
    assert(deck_macro_run_state_complete(&state, first));
    assert(!deck_macro_run_state_is_active(&state));
    assert(!deck_macro_run_state_complete(&state, first));

    second = deck_macro_run_state_begin(&state);
    assert(second != 0 && second != first);
    assert(!deck_macro_run_state_complete(&state, first));
    assert(deck_macro_run_state_is_active(&state));
    assert(deck_macro_run_state_complete(&state, second));

    third = deck_macro_run_state_begin(&state);
    assert(third != 0 && third != second);
    deck_macro_run_state_invalidate(&state);
    assert(!deck_macro_run_state_is_active(&state));
    assert(!deck_macro_run_state_complete(&state, third));

    state.generation = UINT32_MAX;
    assert(deck_macro_run_state_begin(&state) == 1u);
    assert(!deck_macro_run_state_complete(&state, UINT32_MAX));
    assert(deck_macro_run_state_begin(NULL) == 0);
    assert(!deck_macro_run_state_complete(NULL, 1));
    deck_macro_run_state_invalidate(NULL);
    assert(!deck_macro_run_state_is_active(NULL));
    puts("macro run generation and stale completion: OK");
    return 0;
}
