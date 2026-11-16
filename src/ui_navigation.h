#ifndef CDECK_UI_NAVIGATION_H
#define CDECK_UI_NAVIGATION_H

#include "ui_layout.h"

#include <stdbool.h>
#include <stdint.h>

/* Bits 0..14 map to the row-major 5x3 deck slots. */
#define DECK_UI_GRID_FOCUSABLE_MASK ((uint16_t)0x7FFFu)

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
    DECK_UI_NAVIGATION_INVALID_DIRECTION,
    DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT,
    DECK_UI_NAVIGATION_CURRENT_SLOT_DISABLED,
    DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK
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

/*
 * Reconciles focus after a model change (for example, a slot becomes disabled).
 * The nearest focusable slot by row-major index is chosen; ties prefer the
 * following slot. Output is unchanged if no slot is focusable or the mask is
 * invalid.
 */
DeckUiNavigationStatus deck_ui_grid_reconcile_focus(
    uint32_t current_slot, uint16_t focusable_mask, uint32_t *next_slot);

/*
 * Moves along the requested row/column and skips disabled slots. Edges clamp
 * without wrapping. If current_slot is disabled, reconcile first. A zero mask
 * returns NO_FOCUSABLE_SLOT; bits outside slots 0..14 are invalid.
 */
DeckUiNavigationStatus deck_ui_grid_move_focusable(
    uint32_t current_slot, DeckUiDirection direction,
    uint16_t focusable_mask, uint32_t *next_slot);

/* Home/End equivalent that skips disabled slots in the row or whole grid. */
DeckUiNavigationStatus deck_ui_grid_focus_boundary_focusable(
    uint32_t current_slot, bool to_end, bool entire_grid,
    uint16_t focusable_mask, uint32_t *next_slot);

#ifdef __cplusplus
}
#endif

#endif
