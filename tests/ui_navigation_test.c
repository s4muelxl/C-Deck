#include "../src/ui_navigation.h"

#include <assert.h>
#include <stdio.h>

static uint32_t move(uint32_t slot, DeckUiDirection direction) {
    uint32_t next = DECK_UI_GRID_SLOT_COUNT;
    assert(deck_ui_grid_move_focus(slot, direction, &next) == DECK_UI_NAVIGATION_OK);
    assert(next < DECK_UI_GRID_SLOT_COUNT);
    return next;
}

static uint32_t boundary(uint32_t slot, bool to_end, bool entire_grid) {
    uint32_t next = DECK_UI_GRID_SLOT_COUNT;
    assert(deck_ui_grid_focus_boundary(slot, to_end, entire_grid, &next)
           == DECK_UI_NAVIGATION_OK);
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

static void test_home_end_boundaries(void) {
    uint32_t slot;

    for (slot = 0u; slot < DECK_UI_GRID_SLOT_COUNT; ++slot) {
        const uint32_t row_start =
            (slot / DECK_UI_GRID_COLUMNS) * DECK_UI_GRID_COLUMNS;
        const uint32_t row_end = row_start + DECK_UI_GRID_COLUMNS - 1u;

        assert(boundary(slot, false, false) == row_start);
        assert(boundary(slot, true, false) == row_end);
        assert(boundary(slot, false, true) == 0u);
        assert(boundary(slot, true, true) == DECK_UI_GRID_SLOT_COUNT - 1u);
    }
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

    assert(deck_ui_grid_focus_boundary(15u, false, false, &next)
           == DECK_UI_NAVIGATION_INVALID_SLOT);
    assert(next == 77u);
    assert(deck_ui_grid_focus_boundary(0u, false, false, NULL)
           == DECK_UI_NAVIGATION_INVALID_ARGUMENT);
}

static uint16_t disable_slot(uint16_t mask, uint32_t slot) {
    return (uint16_t)(mask & (uint16_t)~(uint16_t)(1u << slot));
}
static uint32_t move_focusable(uint32_t slot, DeckUiDirection direction,
                               uint16_t mask) {
    uint32_t next = DECK_UI_GRID_SLOT_COUNT;
    assert(deck_ui_grid_move_focusable(slot, direction, mask, &next)
           == DECK_UI_NAVIGATION_OK);
    return next;
}
static uint32_t boundary_focusable(uint32_t slot, bool to_end,
                                   bool entire_grid, uint16_t mask) {
    uint32_t next = DECK_UI_GRID_SLOT_COUNT;
    assert(deck_ui_grid_focus_boundary_focusable(
               slot, to_end, entire_grid, mask, &next)
           == DECK_UI_NAVIGATION_OK);
    return next;
}
static void test_disabled_slots_are_skipped(void) {
    uint16_t mask = disable_slot(disable_slot(
        DECK_UI_GRID_FOCUSABLE_MASK, 1u), 2u);
    assert(move_focusable(0u, DECK_UI_DIRECTION_RIGHT, mask) == 3u);
    assert(move_focusable(4u, DECK_UI_DIRECTION_LEFT, mask) == 3u);
    mask = disable_slot(DECK_UI_GRID_FOCUSABLE_MASK, 5u);
    assert(move_focusable(0u, DECK_UI_DIRECTION_DOWN, mask) == 10u);
    assert(move_focusable(0u, DECK_UI_DIRECTION_LEFT, mask) == 0u);
}
static void test_boundaries_skip_disabled_slots(void) {
    uint16_t mask = DECK_UI_GRID_FOCUSABLE_MASK;
    mask = disable_slot(disable_slot(mask, 5u), 6u);
    mask = disable_slot(mask, 9u);
    assert(boundary_focusable(7u, false, false, mask) == 7u);
    assert(boundary_focusable(7u, true, false, mask) == 8u);
    mask = disable_slot(mask, 0u);
    assert(boundary_focusable(7u, false, true, mask) == 1u);
    assert(boundary_focusable(7u, true, true, mask) == 14u);
}
static void test_reconcile_and_no_target(void) {
    uint16_t mask = disable_slot(DECK_UI_GRID_FOCUSABLE_MASK, 7u);
    uint32_t next = 77u;
    assert(deck_ui_grid_reconcile_focus(7u, mask, &next)
           == DECK_UI_NAVIGATION_OK);
    assert(next == 8u);
    next = 77u;
    assert(deck_ui_grid_move_focusable(7u, DECK_UI_DIRECTION_LEFT, mask, &next)
           == DECK_UI_NAVIGATION_CURRENT_SLOT_DISABLED);
    assert(next == 77u);
    assert(deck_ui_grid_reconcile_focus(7u, 0u, &next)
           == DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT);
    assert(next == 77u);
}
static void test_invalid_focusable_mask_preserves_output(void) {
    uint32_t next = 77u;
    uint16_t invalid = (uint16_t)(DECK_UI_GRID_FOCUSABLE_MASK | (1u << 15u));
    assert(deck_ui_grid_move_focusable(0u, DECK_UI_DIRECTION_RIGHT,
                                       invalid, &next)
           == DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK);
    assert(next == 77u);
    assert(deck_ui_grid_focus_boundary_focusable(0u, false, false,
                                                 invalid, &next)
           == DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK);
    assert(next == 77u);
}
int main(void) {
    test_every_slot_and_direction();
    test_edges_do_not_wrap();
    test_home_end_boundaries();
    test_invalid_inputs_preserve_output();
    test_disabled_slots_are_skipped();
    test_boundaries_skip_disabled_slots();
    test_reconcile_and_no_target();
    test_invalid_focusable_mask_preserves_output();
    puts("ui_navigation_test: all checks passed");
    return 0;
}
