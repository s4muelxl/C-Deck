#ifndef CDECK_UI_STATE_H
#define CDECK_UI_STATE_H

#include "ui_navigation.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DECK_UI_SLOT_NONE UINT32_MAX

enum {
    DECK_UI_SLOT_FLAG_NONE = 0u,
    DECK_UI_SLOT_FLAG_FOCUSED = 1u << 0,
    DECK_UI_SLOT_FLAG_SELECTED = 1u << 1,
    DECK_UI_SLOT_FLAG_HOVERED = 1u << 2,
    DECK_UI_SLOT_FLAG_PRESSED = 1u << 3
};

typedef struct DeckUiViewState {
    /* Focus, selection and pointer feedback are intentionally independent. */
    uint32_t focused_slot;
    uint32_t selected_slot;
    uint32_t hovered_slot;
    uint32_t pressed_slot;
} DeckUiViewState;

typedef enum DeckUiViewStatus {
    DECK_UI_VIEW_OK = 0,
    DECK_UI_VIEW_SELECTED,
    DECK_UI_VIEW_POINTER_CANCELLED,
    DECK_UI_VIEW_INVALID_ARGUMENT,
    DECK_UI_VIEW_INVALID_SLOT,
    DECK_UI_VIEW_INVALID_DIRECTION,
    DECK_UI_VIEW_NO_FOCUSABLE_SLOT,
    DECK_UI_VIEW_CURRENT_SLOT_DISABLED,
    DECK_UI_VIEW_INVALID_FOCUSABLE_MASK
} DeckUiViewStatus;

/* Initializes keyboard focus at slot 0; no slot is selected or hovered. */
void deck_ui_view_state_init(DeckUiViewState *state);

/* Moves keyboard focus only. It never changes selection or invokes an action. */
DeckUiViewStatus deck_ui_view_focus(DeckUiViewState *state, uint32_t slot);
DeckUiViewStatus deck_ui_view_move_focus(DeckUiViewState *state,
                                         DeckUiDirection direction);
DeckUiViewStatus deck_ui_view_focus_boundary(DeckUiViewState *state,
                                             int to_end,
                                             int entire_grid);

/*
 * Mask-aware keyboard navigation. Disabled slots are skipped; reconcile after
 * a focused slot becomes disabled. Reconciliation clears stale pressed state
 * but preserves the independent inspector selection.
 */
DeckUiViewStatus deck_ui_view_reconcile_focus(DeckUiViewState *state,
                                              uint16_t focusable_mask);
DeckUiViewStatus deck_ui_view_move_focusable(DeckUiViewState *state,
                                             DeckUiDirection direction,
                                             uint16_t focusable_mask);
DeckUiViewStatus deck_ui_view_focus_boundary_focusable(
    DeckUiViewState *state, int to_end, int entire_grid,
    uint16_t focusable_mask);

/* Selection is an editing/inspector context, never an execution request. */
DeckUiViewStatus deck_ui_view_select(DeckUiViewState *state, uint32_t slot);

/* Pass DECK_UI_SLOT_NONE to clear hover; invalid slots leave state unchanged. */
DeckUiViewStatus deck_ui_view_set_hover(DeckUiViewState *state, uint32_t slot);

/* Pointer press/release models click selection without executing the slot. */
DeckUiViewStatus deck_ui_view_pointer_press(DeckUiViewState *state,
                                            uint32_t slot);

/*
 * Updates hover while a captured pointer is dragging. The pressed visual is
 * shown only while the pointer is over the original pressed slot; moving away
 * clears the visual without losing the captured slot in the input adapter.
 */
DeckUiViewStatus deck_ui_view_pointer_drag(DeckUiViewState *state,
                                           uint32_t pressed_slot,
                                           uint32_t hovered_slot);

DeckUiViewStatus deck_ui_view_pointer_release(DeckUiViewState *state,
                                              uint32_t slot);

/* Clears transient pointer state while preserving focus and selection. */
void deck_ui_view_clear_transient(DeckUiViewState *state);

/* Returns combined state flags; invalid state/slot returns NONE. */
unsigned int deck_ui_view_slot_flags(const DeckUiViewState *state,
                                     uint32_t slot);

#ifdef __cplusplus
}
#endif

#endif
