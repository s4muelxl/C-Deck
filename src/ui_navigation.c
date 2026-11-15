#include "ui_navigation.h"

DeckUiNavigationStatus deck_ui_grid_move_focus(uint32_t current_slot,
                                               DeckUiDirection direction,
                                               uint32_t *next_slot) {
    uint32_t row;
    uint32_t column;
    uint32_t target;

    if (!next_slot) return DECK_UI_NAVIGATION_INVALID_ARGUMENT;
    if (current_slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_NAVIGATION_INVALID_SLOT;
    }

    switch (direction) {
        case DECK_UI_DIRECTION_LEFT:
        case DECK_UI_DIRECTION_RIGHT:
        case DECK_UI_DIRECTION_UP:
        case DECK_UI_DIRECTION_DOWN:
            break;
        default:
            return DECK_UI_NAVIGATION_INVALID_DIRECTION;
    }

    row = current_slot / DECK_UI_GRID_COLUMNS;
    column = current_slot % DECK_UI_GRID_COLUMNS;
    target = current_slot;

    switch (direction) {
        case DECK_UI_DIRECTION_LEFT:
            if (column > 0u) target = current_slot - 1u;
            break;
        case DECK_UI_DIRECTION_RIGHT:
            if (column + 1u < DECK_UI_GRID_COLUMNS) target = current_slot + 1u;
            break;
        case DECK_UI_DIRECTION_UP:
            if (row > 0u) target = current_slot - DECK_UI_GRID_COLUMNS;
            break;
        case DECK_UI_DIRECTION_DOWN:
            if (row + 1u < DECK_UI_GRID_ROWS) target = current_slot + DECK_UI_GRID_COLUMNS;
            break;
        default:
            return DECK_UI_NAVIGATION_INVALID_DIRECTION;
    }

    *next_slot = target;
    return DECK_UI_NAVIGATION_OK;
}
