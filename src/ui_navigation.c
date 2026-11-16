#include "ui_navigation.h"

static int focusable_mask_is_valid(uint16_t focusable_mask) {
    return (focusable_mask & (uint16_t)~DECK_UI_GRID_FOCUSABLE_MASK) == 0u;
}

static int slot_is_focusable(uint16_t focusable_mask, uint32_t slot) {
    return (focusable_mask & (uint16_t)(1u << slot)) != 0u;
}

DeckUiNavigationStatus deck_ui_grid_reconcile_focus(
    uint32_t current_slot,
    uint16_t focusable_mask,
    uint32_t *next_slot) {
    uint32_t distance;

    if (!next_slot) return DECK_UI_NAVIGATION_INVALID_ARGUMENT;
    if (current_slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_NAVIGATION_INVALID_SLOT;
    }
    if (!focusable_mask_is_valid(focusable_mask)) {
        return DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK;
    }
    if (focusable_mask == 0u) {
        return DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT;
    }

    if (slot_is_focusable(focusable_mask, current_slot)) {
        *next_slot = current_slot;
        return DECK_UI_NAVIGATION_OK;
    }

    /* Prefer the following slot when two candidates are equally near. */
    for (distance = 1u; distance < DECK_UI_GRID_SLOT_COUNT; ++distance) {
        const uint32_t forward = current_slot + distance;
        if (forward < DECK_UI_GRID_SLOT_COUNT &&
            slot_is_focusable(focusable_mask, forward)) {
            *next_slot = forward;
            return DECK_UI_NAVIGATION_OK;
        }
        if (current_slot >= distance) {
            const uint32_t backward = current_slot - distance;
            if (slot_is_focusable(focusable_mask, backward)) {
                *next_slot = backward;
                return DECK_UI_NAVIGATION_OK;
            }
        }
    }

    return DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT;
}

DeckUiNavigationStatus deck_ui_grid_move_focusable(
    uint32_t current_slot,
    DeckUiDirection direction,
    uint16_t focusable_mask,
    uint32_t *next_slot) {
    uint32_t candidate;

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
    if (!focusable_mask_is_valid(focusable_mask)) {
        return DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK;
    }
    if (focusable_mask == 0u) {
        return DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT;
    }
    if (!slot_is_focusable(focusable_mask, current_slot)) {
        return DECK_UI_NAVIGATION_CURRENT_SLOT_DISABLED;
    }

    candidate = current_slot;
    switch (direction) {
        case DECK_UI_DIRECTION_LEFT:
            while (candidate % DECK_UI_GRID_COLUMNS > 0u) {
                --candidate;
                if (slot_is_focusable(focusable_mask, candidate)) {
                    *next_slot = candidate;
                    return DECK_UI_NAVIGATION_OK;
                }
            }
            break;
        case DECK_UI_DIRECTION_RIGHT:
            while (candidate % DECK_UI_GRID_COLUMNS + 1u <
                   DECK_UI_GRID_COLUMNS) {
                ++candidate;
                if (slot_is_focusable(focusable_mask, candidate)) {
                    *next_slot = candidate;
                    return DECK_UI_NAVIGATION_OK;
                }
            }
            break;
        case DECK_UI_DIRECTION_UP:
            while (candidate >= DECK_UI_GRID_COLUMNS) {
                candidate -= DECK_UI_GRID_COLUMNS;
                if (slot_is_focusable(focusable_mask, candidate)) {
                    *next_slot = candidate;
                    return DECK_UI_NAVIGATION_OK;
                }
            }
            break;
        case DECK_UI_DIRECTION_DOWN:
            while (candidate + DECK_UI_GRID_COLUMNS <
                   DECK_UI_GRID_SLOT_COUNT) {
                candidate += DECK_UI_GRID_COLUMNS;
                if (slot_is_focusable(focusable_mask, candidate)) {
                    *next_slot = candidate;
                    return DECK_UI_NAVIGATION_OK;
                }
            }
            break;
        default:
            return DECK_UI_NAVIGATION_INVALID_DIRECTION;
    }

    /* Preserve the established no-wrap, edge-clamped navigation behavior. */
    *next_slot = current_slot;
    return DECK_UI_NAVIGATION_OK;
}

DeckUiNavigationStatus deck_ui_grid_focus_boundary_focusable(
    uint32_t current_slot,
    bool to_end,
    bool entire_grid,
    uint16_t focusable_mask,
    uint32_t *next_slot) {
    uint32_t first;
    uint32_t last;
    uint32_t candidate;

    if (!next_slot) return DECK_UI_NAVIGATION_INVALID_ARGUMENT;
    if (current_slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_NAVIGATION_INVALID_SLOT;
    }
    if (!focusable_mask_is_valid(focusable_mask)) {
        return DECK_UI_NAVIGATION_INVALID_FOCUSABLE_MASK;
    }
    if (focusable_mask == 0u) {
        return DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT;
    }

    if (entire_grid) {
        first = 0u;
        last = DECK_UI_GRID_SLOT_COUNT - 1u;
    } else {
        first = (current_slot / DECK_UI_GRID_COLUMNS) * DECK_UI_GRID_COLUMNS;
        last = first + DECK_UI_GRID_COLUMNS - 1u;
    }

    if (to_end) {
        candidate = last;
        for (;;) {
            if (slot_is_focusable(focusable_mask, candidate)) {
                *next_slot = candidate;
                return DECK_UI_NAVIGATION_OK;
            }
            if (candidate == first) break;
            --candidate;
        }
    } else {
        for (candidate = first; candidate <= last; ++candidate) {
            if (slot_is_focusable(focusable_mask, candidate)) {
                *next_slot = candidate;
                return DECK_UI_NAVIGATION_OK;
            }
        }
    }

    return DECK_UI_NAVIGATION_NO_FOCUSABLE_SLOT;
}

DeckUiNavigationStatus deck_ui_grid_move_focus(
    uint32_t current_slot,
    DeckUiDirection direction,
    uint32_t *next_slot) {
    return deck_ui_grid_move_focusable(current_slot, direction,
                                       DECK_UI_GRID_FOCUSABLE_MASK,
                                       next_slot);
}

DeckUiNavigationStatus deck_ui_grid_focus_boundary(
    uint32_t current_slot,
    bool to_end,
    bool entire_grid,
    uint32_t *next_slot) {
    return deck_ui_grid_focus_boundary_focusable(
        current_slot, to_end, entire_grid, DECK_UI_GRID_FOCUSABLE_MASK,
        next_slot);
}
