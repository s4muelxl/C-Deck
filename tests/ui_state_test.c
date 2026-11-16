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

int main(void) {
    test_initial_state_and_independent_focus();
    test_keyboard_selection_and_boundary();
    test_pointer_click_selects_without_execution();
    test_pointer_cancel_does_not_change_selection();
    test_transient_clear_preserves_context();
    test_invalid_inputs_preserve_state();
    puts("ui_state_test: all checks passed");
    return 0;
}
