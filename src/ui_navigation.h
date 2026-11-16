#ifndef CDECK_UI_NAVIGATION_H
#define CDECK_UI_NAVIGATION_H

#include "ui_layout.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DeckUiDirection {
    DECK_UI_DIRECTION_LEFT = 0,
    DECK_UI_DIRECTION_RIGHT,
    DECK_UI_DIRECTION_UP,
    DECK_UI_DIRECTION_DOWN
} DeckUiDirection;

typedef enum DeckUiNavigationStatus {
    DECK_UI_NAVIGATION_OK = 0,
    DECK_UI_NAVIGATION_INVALID_ARGUMENT,
    DECK_UI_NAVIGATION_INVALID_SLOT,
    DECK_UI_NAVIGATION_INVALID_DIRECTION
} DeckUiNavigationStatus;

/*
 * Moves keyboard focus inside the fixed 5x3 deck grid.
 * Movement is clamped at row/column edges (no wrap), so focus never jumps
 * unexpectedly to another edge of the grid. This function only computes
 * focus; it never selects a slot or invokes its action.
 *
 * On error, next_slot is left unchanged.
 */
DeckUiNavigationStatus deck_ui_grid_move_focus(uint32_t current_slot,
                                               DeckUiDirection direction,
                                               uint32_t *next_slot);

/*
 * Moves focus to the first/last slot in the current row, or in the entire
 * grid when entire_grid is true. to_end selects the last boundary.
 * This computes focus only; it never selects a slot or invokes its action.
 * On error, next_slot is left unchanged.
 */
DeckUiNavigationStatus deck_ui_grid_focus_boundary(uint32_t current_slot,
                                                   bool to_end,
                                                   bool entire_grid,
                                                   uint32_t *next_slot);

#ifdef __cplusplus
}
#endif

#endif
