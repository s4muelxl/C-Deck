#include "../src/ui_navigation.h"

#include <assert.h>
#include <stdio.h>

static uint32_t move(uint32_t slot, DeckUiDirection direction) {
    uint32_t next = DECK_UI_GRID_SLOT_COUNT;
    assert(deck_ui_grid_move_focus(slot, direction, &next) == DECK_UI_NAVIGATION_OK);
    assert(next < DECK_UI_GRID_SLOT_COUNT);
    return next;
}

static void test_every_slot_and_direction(void) {
    uint32_t slot;

    for (slot = 0u; slot < DECK_UI_GRID_SLOT_COUNT; ++slot) {
        const uint32_t row = slot / DECK_UI_GRID_COLUMNS;
        const uint32_t column = slot % DECK_UI_GRID_COLUMNS;
        const uint32_t expected_left = column > 0u ? slot - 1u : slot;
        const uint32_t expected_right =
            column + 1u < DECK_UI_GRID_COLUMNS ? slot + 1u : slot;
        const uint32_t expected_up =
            row > 0u ? slot - DECK_UI_GRID_COLUMNS : slot;
        const uint32_t expected_down =
            row + 1u < DECK_UI_GRID_ROWS ? slot + DECK_UI_GRID_COLUMNS : slot;

        assert(move(slot, DECK_UI_DIRECTION_LEFT) == expected_left);
        assert(move(slot, DECK_UI_DIRECTION_RIGHT) == expected_right);
        assert(move(slot, DECK_UI_DIRECTION_UP) == expected_up);
        assert(move(slot, DECK_UI_DIRECTION_DOWN) == expected_down);
    }
}

static void test_edges_do_not_wrap(void) {
    assert(move(0u, DECK_UI_DIRECTION_LEFT) == 0u);
    assert(move(4u, DECK_UI_DIRECTION_RIGHT) == 4u);
    assert(move(0u, DECK_UI_DIRECTION_UP) == 0u);
    assert(move(10u, DECK_UI_DIRECTION_DOWN) == 10u);
    assert(move(14u, DECK_UI_DIRECTION_RIGHT) == 14u);
    assert(move(14u, DECK_UI_DIRECTION_DOWN) == 14u);
}

static void test_invalid_inputs_preserve_output(void) {
    uint32_t next = 77u;

    assert(deck_ui_grid_move_focus(15u, DECK_UI_DIRECTION_LEFT, &next)
           == DECK_UI_NAVIGATION_INVALID_SLOT);
    assert(next == 77u);

    assert(deck_ui_grid_move_focus(0u, (DeckUiDirection)99, &next)
           == DECK_UI_NAVIGATION_INVALID_DIRECTION);
    assert(next == 77u);

    assert(deck_ui_grid_move_focus(0u, DECK_UI_DIRECTION_LEFT, NULL)
           == DECK_UI_NAVIGATION_INVALID_ARGUMENT);
}

int main(void) {
    test_every_slot_and_direction();
    test_edges_do_not_wrap();
    test_invalid_inputs_preserve_output();
    puts("ui_navigation_test: all checks passed");
    return 0;
}
