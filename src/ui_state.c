#include "ui_state.h"

static int valid_slot_or_none(uint32_t slot) {
    return slot == DECK_UI_SLOT_NONE || slot < DECK_UI_GRID_SLOT_COUNT;
}

void deck_ui_view_state_init(DeckUiViewState *state) {
    if (!state) return;
    state->focused_slot = 0u;
    state->selected_slot = DECK_UI_SLOT_NONE;
    state->hovered_slot = DECK_UI_SLOT_NONE;
    state->pressed_slot = DECK_UI_SLOT_NONE;
}

DeckUiViewStatus deck_ui_view_focus(DeckUiViewState *state, uint32_t slot) {
    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (slot >= DECK_UI_GRID_SLOT_COUNT) return DECK_UI_VIEW_INVALID_SLOT;
    state->focused_slot = slot;
    state->pressed_slot = DECK_UI_SLOT_NONE;
    return DECK_UI_VIEW_OK;
}

DeckUiViewStatus deck_ui_view_move_focus(DeckUiViewState *state,
                                         DeckUiDirection direction) {
    uint32_t next_slot;
    DeckUiNavigationStatus status;

    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (state->focused_slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_VIEW_INVALID_SLOT;
    }

    status = deck_ui_grid_move_focus(state->focused_slot, direction, &next_slot);
    if (status == DECK_UI_NAVIGATION_INVALID_DIRECTION) {
        return DECK_UI_VIEW_INVALID_DIRECTION;
    }
    if (status != DECK_UI_NAVIGATION_OK) return DECK_UI_VIEW_INVALID_SLOT;

    state->focused_slot = next_slot;
    state->pressed_slot = DECK_UI_SLOT_NONE;
    return DECK_UI_VIEW_OK;
}

DeckUiViewStatus deck_ui_view_focus_boundary(DeckUiViewState *state,
                                             int to_end,
                                             int entire_grid) {
    uint32_t next_slot;
    DeckUiNavigationStatus status;

    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (state->focused_slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_VIEW_INVALID_SLOT;
    }

    status = deck_ui_grid_focus_boundary(state->focused_slot,
                                         to_end != 0,
                                         entire_grid != 0,
                                         &next_slot);
    if (status != DECK_UI_NAVIGATION_OK) return DECK_UI_VIEW_INVALID_SLOT;
    state->focused_slot = next_slot;
    state->pressed_slot = DECK_UI_SLOT_NONE;
    return DECK_UI_VIEW_OK;
}

DeckUiViewStatus deck_ui_view_select(DeckUiViewState *state, uint32_t slot) {
    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (slot >= DECK_UI_GRID_SLOT_COUNT) return DECK_UI_VIEW_INVALID_SLOT;
    state->focused_slot = slot;
    state->selected_slot = slot;
    state->pressed_slot = DECK_UI_SLOT_NONE;
    return DECK_UI_VIEW_SELECTED;
}

DeckUiViewStatus deck_ui_view_set_hover(DeckUiViewState *state, uint32_t slot) {
    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (!valid_slot_or_none(slot)) return DECK_UI_VIEW_INVALID_SLOT;
    state->hovered_slot = slot;
    return DECK_UI_VIEW_OK;
}

DeckUiViewStatus deck_ui_view_pointer_press(DeckUiViewState *state,
                                            uint32_t slot) {
    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (slot >= DECK_UI_GRID_SLOT_COUNT) return DECK_UI_VIEW_INVALID_SLOT;
    state->focused_slot = slot;
    state->hovered_slot = slot;
    state->pressed_slot = slot;
    return DECK_UI_VIEW_OK;
}

DeckUiViewStatus deck_ui_view_pointer_release(DeckUiViewState *state,
                                              uint32_t slot) {
    uint32_t pressed_slot;

    if (!state) return DECK_UI_VIEW_INVALID_ARGUMENT;
    if (!valid_slot_or_none(slot)) return DECK_UI_VIEW_INVALID_SLOT;

    pressed_slot = state->pressed_slot;
    state->pressed_slot = DECK_UI_SLOT_NONE;
    state->hovered_slot = slot;
    if (pressed_slot != DECK_UI_SLOT_NONE && pressed_slot == slot) {
        state->focused_slot = slot;
        state->selected_slot = slot;
        return DECK_UI_VIEW_SELECTED;
    }
    return DECK_UI_VIEW_POINTER_CANCELLED;
}

void deck_ui_view_clear_transient(DeckUiViewState *state) {
    if (!state) return;
    state->hovered_slot = DECK_UI_SLOT_NONE;
    state->pressed_slot = DECK_UI_SLOT_NONE;
}

unsigned int deck_ui_view_slot_flags(const DeckUiViewState *state,
                                     uint32_t slot) {
    unsigned int flags = DECK_UI_SLOT_FLAG_NONE;

    if (!state || slot >= DECK_UI_GRID_SLOT_COUNT) return flags;
    if (state->focused_slot == slot) flags |= DECK_UI_SLOT_FLAG_FOCUSED;
    if (state->selected_slot == slot) flags |= DECK_UI_SLOT_FLAG_SELECTED;
    if (state->hovered_slot == slot) flags |= DECK_UI_SLOT_FLAG_HOVERED;
    if (state->pressed_slot == slot) flags |= DECK_UI_SLOT_FLAG_PRESSED;
    return flags;
}
