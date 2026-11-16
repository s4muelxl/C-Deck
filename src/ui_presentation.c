#include "ui_presentation.h"

static int valid_model_state(DeckUiVisualState state) {
    return state == DECK_UI_STATE_NORMAL ||
           state == DECK_UI_STATE_DISABLED ||
           state == DECK_UI_STATE_ERROR ||
           state == DECK_UI_STATE_WARNING;
}

DeckUiPresentationStatus deck_ui_presentation_resolve_slot(
    const DeckUiViewState *view_state,
    uint32_t slot,
    DeckUiVisualState model_state,
    DeckUiPresentationState *presentation) {
    unsigned int flags;
    DeckUiPresentationState resolved;

    if (!view_state || !presentation) {
        return DECK_UI_PRESENTATION_INVALID_ARGUMENT;
    }
    if (slot >= DECK_UI_GRID_SLOT_COUNT) {
        return DECK_UI_PRESENTATION_INVALID_SLOT;
    }
    if (!valid_model_state(model_state)) {
        return DECK_UI_PRESENTATION_INVALID_MODEL_STATE;
    }

    flags = deck_ui_view_slot_flags(view_state, slot);
    resolved.visual_state = model_state;

    if (model_state == DECK_UI_STATE_NORMAL) {
        if ((flags & DECK_UI_SLOT_FLAG_PRESSED) != 0u) {
            resolved.visual_state = DECK_UI_STATE_PRESSED;
        } else if ((flags & DECK_UI_SLOT_FLAG_SELECTED) != 0u) {
            resolved.visual_state = DECK_UI_STATE_SELECTED;
        } else if ((flags & DECK_UI_SLOT_FLAG_HOVERED) != 0u) {
            resolved.visual_state = DECK_UI_STATE_HOVER;
        }
    }

    resolved.focused =
        model_state != DECK_UI_STATE_DISABLED &&
        (flags & DECK_UI_SLOT_FLAG_FOCUSED) != 0u;

    *presentation = resolved;
    return DECK_UI_PRESENTATION_OK;
}
