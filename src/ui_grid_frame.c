#include "ui_grid_frame.h"

#include <stdint.h>

static int optional_slot_is_valid(uint32_t slot) {
    return slot == DECK_UI_SLOT_NONE || slot < DECK_UI_GRID_SLOT_COUNT;
}

static int view_state_is_valid(const DeckUiViewState *state) {
    return state &&
           state->focused_slot < DECK_UI_GRID_SLOT_COUNT &&
           optional_slot_is_valid(state->selected_slot) &&
           optional_slot_is_valid(state->hovered_slot) &&
           optional_slot_is_valid(state->pressed_slot);
}

static int layout_is_valid_and_ordered(const DeckUiLayout *layout) {
    uint32_t index;

    if (!layout) return 0;

    /*
     * deck_ui_grid_hit_test validates the complete grid (mode, slot count,
     * bounds, square dimensions and overlap). Checking each slot center also
     * guarantees that the public slot array is in row-major order.
     */
    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiRect *slot = &layout->slots[index];
        const int64_t center_x = (int64_t)slot->x + slot->width / 2;
        const int64_t center_y = (int64_t)slot->y + slot->height / 2;
        uint32_t hit = DECK_UI_GRID_SLOT_COUNT;

        if (center_x < INT32_MIN || center_x > INT32_MAX ||
            center_y < INT32_MIN || center_y > INT32_MAX) {
            return 0;
        }

        if (deck_ui_grid_hit_test(layout,
                                  (int32_t)center_x,
                                  (int32_t)center_y,
                                  &hit) != DECK_UI_HIT_TEST_HIT ||
            hit != index) {
            return 0;
        }
    }

    return 1;
}

DeckUiGridFrameStatus deck_ui_grid_frame_build(
    const DeckUiLayout *layout,
    const DeckUiViewState *view_state,
    const DeckUiPalette *palette,
    const DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT],
    DeckUiGridFrame *frame) {
    DeckUiGridFrame resolved;
    uint32_t index;

    if (!layout || !view_state || !palette || !frame) {
        return DECK_UI_GRID_FRAME_INVALID_ARGUMENT;
    }
    if (!layout_is_valid_and_ordered(layout)) {
        return DECK_UI_GRID_FRAME_INVALID_LAYOUT;
    }
    if (!view_state_is_valid(view_state)) {
        return DECK_UI_GRID_FRAME_INVALID_VIEW_STATE;
    }

    resolved.layout_mode = layout->mode;
    resolved.slot_count = DECK_UI_GRID_SLOT_COUNT;

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiVisualState model_state =
            model_states ? model_states[index] : DECK_UI_STATE_NORMAL;
        DeckUiPresentationState presentation;
        DeckUiPresentationStatus presentation_status;

        presentation_status = deck_ui_presentation_resolve_slot(
            view_state, index, model_state, &presentation);
        if (presentation_status == DECK_UI_PRESENTATION_INVALID_MODEL_STATE) {
            return DECK_UI_GRID_FRAME_INVALID_MODEL_STATE;
        }
        if (presentation_status != DECK_UI_PRESENTATION_OK) {
            return DECK_UI_GRID_FRAME_INVALID_VIEW_STATE;
        }

        resolved.slots[index].bounds = layout->slots[index];
        resolved.slots[index].presentation = presentation;
        resolved.slots[index].style = deck_ui_style_for_interaction(
            palette, presentation.visual_state, presentation.focused);
    }

    *frame = resolved;
    return DECK_UI_GRID_FRAME_OK;
}
