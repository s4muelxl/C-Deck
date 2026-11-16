#include "../src/ui_state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_initial_state_and_independent_focus(void) {
    DeckUiViewState state;

    deck_ui_view_state_init(&state);
    assert(state.focused_slot == 0u);
    assert(state.selected_slot == DECK_UI_SLOT_NONE);
    assert(state.hovered_slot == DECK_UI_SLOT_NONE);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);

    assert(deck_ui_view_move_focus(&state, DECK_UI_DIRECTION_RIGHT)
           == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 1u);
    assert(state.selected_slot == DECK_UI_SLOT_NONE);
    assert((deck_ui_view_slot_flags(&state, 1u) & DECK_UI_SLOT_FLAG_FOCUSED) != 0u);
    assert((deck_ui_view_slot_flags(&state, 1u) & DECK_UI_SLOT_FLAG_SELECTED) == 0u);
}

static void test_keyboard_selection_and_boundary(void) {
    DeckUiViewState state;

    deck_ui_view_state_init(&state);
    assert(deck_ui_view_focus(&state, 7u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_select(&state, 7u) == DECK_UI_VIEW_SELECTED);
    assert(state.focused_slot == 7u && state.selected_slot == 7u);
    assert((deck_ui_view_slot_flags(&state, 7u)
            & (DECK_UI_SLOT_FLAG_FOCUSED | DECK_UI_SLOT_FLAG_SELECTED))
           == (DECK_UI_SLOT_FLAG_FOCUSED | DECK_UI_SLOT_FLAG_SELECTED));

    assert(deck_ui_view_focus_boundary(&state, 0, 0) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 5u);
    assert(state.selected_slot == 7u);
    assert(deck_ui_view_focus_boundary(&state, 1, 1) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 14u);
    assert(state.selected_slot == 7u);
}

static void test_pointer_click_selects_without_execution(void) {
    DeckUiViewState state;

    deck_ui_view_state_init(&state);
    assert(deck_ui_view_set_hover(&state, 4u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_pointer_press(&state, 4u) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 4u);
    assert(state.selected_slot == DECK_UI_SLOT_NONE);
    assert((deck_ui_view_slot_flags(&state, 4u)
            & (DECK_UI_SLOT_FLAG_FOCUSED | DECK_UI_SLOT_FLAG_HOVERED
               | DECK_UI_SLOT_FLAG_PRESSED))
           == (DECK_UI_SLOT_FLAG_FOCUSED | DECK_UI_SLOT_FLAG_HOVERED
               | DECK_UI_SLOT_FLAG_PRESSED));

    assert(deck_ui_view_pointer_release(&state, 4u) == DECK_UI_VIEW_SELECTED);
    assert(state.selected_slot == 4u);
    assert(state.focused_slot == 4u);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);
    assert(state.hovered_slot == 4u);
}

static void test_pointer_cancel_does_not_change_selection(void) {
    DeckUiViewState state;

    deck_ui_view_state_init(&state);
    assert(deck_ui_view_select(&state, 2u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_pointer_press(&state, 5u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_pointer_release(&state, 6u)
           == DECK_UI_VIEW_POINTER_CANCELLED);
    assert(state.selected_slot == 2u);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);
    assert(state.hovered_slot == 6u);

    assert(deck_ui_view_pointer_press(&state, 5u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_pointer_release(&state, DECK_UI_SLOT_NONE)
           == DECK_UI_VIEW_POINTER_CANCELLED);
    assert(state.selected_slot == 2u);
    assert(state.hovered_slot == DECK_UI_SLOT_NONE);
}

static void test_transient_clear_preserves_context(void) {
    DeckUiViewState state;

    deck_ui_view_state_init(&state);
    assert(deck_ui_view_select(&state, 9u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_set_hover(&state, 10u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_pointer_press(&state, 10u) == DECK_UI_VIEW_OK);
    deck_ui_view_clear_transient(&state);

    assert(state.focused_slot == 10u);
    assert(state.selected_slot == 9u);
    assert(state.hovered_slot == DECK_UI_SLOT_NONE);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);
    assert(deck_ui_view_slot_flags(&state, 10u) == DECK_UI_SLOT_FLAG_FOCUSED);

    assert(deck_ui_view_pointer_press(&state, 11u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_focus(&state, 12u) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 12u);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);
}

static void test_invalid_inputs_preserve_state(void) {
    DeckUiViewState state;
    DeckUiViewState before;

    deck_ui_view_state_init(&state);
    assert(deck_ui_view_select(&state, 3u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_set_hover(&state, 4u) == DECK_UI_VIEW_OK);
    assert(deck_ui_view_pointer_press(&state, 4u) == DECK_UI_VIEW_OK);
    before = state;

    assert(deck_ui_view_focus(&state, 15u) == DECK_UI_VIEW_INVALID_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_select(&state, 15u) == DECK_UI_VIEW_INVALID_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_set_hover(&state, 15u) == DECK_UI_VIEW_INVALID_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_pointer_press(&state, 15u) == DECK_UI_VIEW_INVALID_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_pointer_release(&state, 15u) == DECK_UI_VIEW_INVALID_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_move_focus(&state, (DeckUiDirection)99)
           == DECK_UI_VIEW_INVALID_DIRECTION);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_slot_flags(&state, 15u) == DECK_UI_SLOT_FLAG_NONE);

    assert(deck_ui_view_focus(NULL, 0u) == DECK_UI_VIEW_INVALID_ARGUMENT);
    assert(deck_ui_view_move_focus(NULL, DECK_UI_DIRECTION_LEFT)
           == DECK_UI_VIEW_INVALID_ARGUMENT);
    assert(deck_ui_view_select(NULL, 0u) == DECK_UI_VIEW_INVALID_ARGUMENT);
    assert(deck_ui_view_set_hover(NULL, DECK_UI_SLOT_NONE)
           == DECK_UI_VIEW_INVALID_ARGUMENT);
    assert(deck_ui_view_pointer_press(NULL, 0u) == DECK_UI_VIEW_INVALID_ARGUMENT);
    assert(deck_ui_view_pointer_release(NULL, 0u) == DECK_UI_VIEW_INVALID_ARGUMENT);
}

static uint16_t disable_view_slot(uint16_t mask, uint32_t slot) {
    return (uint16_t)(mask & (uint16_t)~(uint16_t)(1u << slot));
}
static void test_view_skips_disabled_and_preserves_selection(void) {
    DeckUiViewState state;
    uint16_t mask = disable_view_slot(disable_view_slot(
        DECK_UI_GRID_FOCUSABLE_MASK, 1u), 2u);
    deck_ui_view_state_init(&state);
    assert(deck_ui_view_move_focusable(&state, DECK_UI_DIRECTION_RIGHT, mask)
           == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 3u);
    assert(state.selected_slot == DECK_UI_SLOT_NONE);
    assert(deck_ui_view_select(&state, 7u) == DECK_UI_VIEW_SELECTED);
    mask = disable_view_slot(DECK_UI_GRID_FOCUSABLE_MASK, 7u);
    assert(deck_ui_view_reconcile_focus(&state, mask) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 8u);
    assert(state.selected_slot == 7u);
}
static void test_view_boundary_and_errors_are_safe(void) {
    DeckUiViewState state, before;
    uint16_t mask = DECK_UI_GRID_FOCUSABLE_MASK;
    deck_ui_view_state_init(&state);
    assert(deck_ui_view_focus(&state, 7u) == DECK_UI_VIEW_OK);
    mask = disable_view_slot(disable_view_slot(mask, 5u), 6u);
    assert(deck_ui_view_focus_boundary_focusable(&state, 0, 0, mask)
           == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 7u);
    mask = disable_view_slot(mask, 9u);
    assert(deck_ui_view_focus_boundary_focusable(&state, 1, 0, mask)
           == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 8u);
    before = state;
    assert(deck_ui_view_move_focusable(&state, DECK_UI_DIRECTION_LEFT, 0u)
           == DECK_UI_VIEW_NO_FOCUSABLE_SLOT);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_focus_boundary_focusable(
        &state, 0, 0, (uint16_t)(DECK_UI_GRID_FOCUSABLE_MASK | (1u << 15u)))
        == DECK_UI_VIEW_INVALID_FOCUSABLE_MASK);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
}
static void test_disabled_current_requires_reconciliation(void) {
    DeckUiViewState state, before;
    uint16_t mask;
    deck_ui_view_state_init(&state);
    assert(deck_ui_view_select(&state, 7u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_pointer_press(&state, 7u) == DECK_UI_VIEW_OK);
    before = state;
    mask = disable_view_slot(DECK_UI_GRID_FOCUSABLE_MASK, 7u);
    assert(deck_ui_view_move_focusable(&state, DECK_UI_DIRECTION_LEFT, mask)
           == DECK_UI_VIEW_CURRENT_SLOT_DISABLED);
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(deck_ui_view_reconcile_focus(&state, mask) == DECK_UI_VIEW_OK);
    assert(state.focused_slot == 8u);
    assert(state.selected_slot == 7u);
    assert(state.pressed_slot == DECK_UI_SLOT_NONE);
}
int main(void) {
    test_initial_state_and_independent_focus();
    test_keyboard_selection_and_boundary();
    test_pointer_click_selects_without_execution();
    test_pointer_cancel_does_not_change_selection();
    test_transient_clear_preserves_context();
    test_invalid_inputs_preserve_state();
    test_view_skips_disabled_and_preserves_selection();
    test_view_boundary_and_errors_are_safe();
    test_disabled_current_requires_reconciliation();
    puts("ui_state_test: all checks passed");
    return 0;
}
