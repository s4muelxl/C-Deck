#ifndef CDECK_UI_GRID_FRAME_H
#define CDECK_UI_GRID_FRAME_H

#include "ui_layout.h"
#include "ui_presentation.h"
#include "ui_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DeckUiGridSlotFrame {
    DeckUiRect bounds;
    DeckUiPresentationState presentation;
    DeckUiStyle style;
} DeckUiGridSlotFrame;

typedef struct DeckUiGridFrame {
    DeckUiLayoutMode layout_mode;
    uint32_t slot_count;
    DeckUiGridSlotFrame slots[DECK_UI_GRID_SLOT_COUNT];
} DeckUiGridFrame;

typedef enum DeckUiGridFrameStatus {
    DECK_UI_GRID_FRAME_OK = 0,
    DECK_UI_GRID_FRAME_INVALID_ARGUMENT,
    DECK_UI_GRID_FRAME_INVALID_LAYOUT,
    DECK_UI_GRID_FRAME_INVALID_VIEW_STATE,
    DECK_UI_GRID_FRAME_INVALID_MODEL_STATE
} DeckUiGridFrameStatus;

/*
 * Derives a row-major keyboard-focus mask from the model states used by the
 * renderer. Only DISABLED is excluded; ERROR and WARNING remain inspectable.
 * NULL means all 15 slots are focusable. Output is unchanged on failure.
 */
DeckUiGridFrameStatus deck_ui_grid_focusable_mask_from_states(
    const DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT],
    uint16_t *focusable_mask);

/*
 * Builds a renderer-ready snapshot for all 15 slots.
 *
 * model_states may be NULL, in which case all slots are treated as NORMAL.
 * When supplied, each entry must be one of NORMAL, DISABLED, ERROR or WARNING;
 * transient pointer/keyboard states come from view_state instead.
 *
 * The layout must contain 15 valid, non-overlapping, row-major square slots.
 * The view state must have a valid focused slot; selected/hovered/pressed may
 * be DECK_UI_SLOT_NONE or a valid slot index.
 *
 * This function performs no drawing, allocation, persistence or action
 * execution and has no Win32 dependency. The output is unchanged on failure.
 */
DeckUiGridFrameStatus deck_ui_grid_frame_build(
    const DeckUiLayout *layout,
    const DeckUiViewState *view_state,
    const DeckUiPalette *palette,
    const DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT],
    DeckUiGridFrame *frame);

#ifdef __cplusplus
}
#endif

#endif
