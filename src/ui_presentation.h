#ifndef CDECK_UI_PRESENTATION_H
#define CDECK_UI_PRESENTATION_H

#include "ui_state.h"
#include "ui_theme.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DeckUiPresentationState {
    /* Primary state is semantic; focus is an independent renderer overlay. */
    DeckUiVisualState visual_state;
    int focused;
} DeckUiPresentationState;

typedef enum DeckUiPresentationStatus {
    DECK_UI_PRESENTATION_OK = 0,
    DECK_UI_PRESENTATION_INVALID_ARGUMENT,
    DECK_UI_PRESENTATION_INVALID_SLOT,
    DECK_UI_PRESENTATION_INVALID_MODEL_STATE
} DeckUiPresentationStatus;

/*
 * Resolves a slot's view state into a renderer-ready semantic state.
 *
 * model_state accepts only NORMAL, DISABLED, ERROR or WARNING. Those model
 * states override pointer feedback. For NORMAL, precedence is PRESSED,
 * SELECTED, HOVERED, then NORMAL. Keyboard focus remains independent and is
 * suppressed for DISABLED controls, which must not enter the tab sequence.
 *
 * This function does not draw, persist, execute actions or call Win32. The
 * output is unchanged when any argument is invalid.
 */
DeckUiPresentationStatus deck_ui_presentation_resolve_slot(
    const DeckUiViewState *view_state,
    uint32_t slot,
    DeckUiVisualState model_state,
    DeckUiPresentationState *presentation);

#ifdef __cplusplus
}
#endif

#endif
