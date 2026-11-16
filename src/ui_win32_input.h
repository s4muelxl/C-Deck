#ifndef CDECK_UI_WIN32_INPUT_H
#define CDECK_UI_WIN32_INPUT_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#include "ui_grid_frame.h"
#include "ui_state.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DECK_UI_WIN32_MODIFIER_NONE = 0u,
    DECK_UI_WIN32_MODIFIER_CONTROL = 1u << 0,
    DECK_UI_WIN32_MODIFIER_META = 1u << 1
};

typedef enum DeckUiWin32PressSource {
    DECK_UI_WIN32_PRESS_NONE = 0,
    DECK_UI_WIN32_PRESS_MOUSE,
    DECK_UI_WIN32_PRESS_KEYBOARD
} DeckUiWin32PressSource;

typedef struct DeckUiWin32Input {
    uint32_t mouse_pressed_slot;
    uint32_t keyboard_pressed_slot;
    DeckUiWin32PressSource press_source;
} DeckUiWin32Input;

typedef enum DeckUiWin32InputStatus {
    DECK_UI_WIN32_INPUT_OK = 0,
    DECK_UI_WIN32_INPUT_UNHANDLED,
    DECK_UI_WIN32_INPUT_INVALID_ARGUMENT,
    DECK_UI_WIN32_INPUT_INVALID_STATE,
    DECK_UI_WIN32_INPUT_INVALID_LAYOUT,
    DECK_UI_WIN32_INPUT_INVALID_MODEL_STATE,
    DECK_UI_WIN32_INPUT_NO_FOCUSABLE_SLOT
} DeckUiWin32InputStatus;

typedef struct DeckUiWin32InputResult {
    int handled;
    int invalidate;
    int focus_changed;
    int hover_changed;
    int pressed_changed;
    int selection_changed;
    int capture_mouse;
    int release_mouse_capture;
    int request_mouse_leave_tracking;
    uint32_t selected_slot;
} DeckUiWin32InputResult;

/*
 * Initializes the adapter. The adapter owns only input-source bookkeeping;
 * DeckUiViewState remains the single source of focus/selection/hover/pressed.
 */
void deck_ui_win32_input_init(DeckUiWin32Input *input);

/*
 * Translates a subset of Win32 messages into the existing 5x3 view-state
 * contract. Mouse coordinates are client pixels and are converted to DIP.
 * dpi=0 means 96; otherwise dpi must be in [48, 768].
 *
 * Supported messages: WM_MOUSEMOVE, WM_MOUSELEAVE, WM_LBUTTONDOWN,
 * WM_LBUTTONUP, WM_CAPTURECHANGED, WM_CANCELMODE, WM_KILLFOCUS, WM_KEYDOWN and WM_KEYUP.
 * Home/End use the whole grid when Control or Meta is present in modifiers.
 *
 * The adapter never executes an action or persists configuration. Mouse
 * release and Enter/Space only select a slot for the Inspector/editor.
 *
 * The window procedure remains responsible for TrackMouseEvent, SetCapture
 * and ReleaseCapture, using the corresponding flags in the result. The
 * caller must pass the current window DPI and current model states.
 *
 * On invalid arguments, model state or layout, input/view/result are unchanged.
 * For an unrecognized message, UNHANDLED is returned and result.handled is 0.
 */
DeckUiWin32InputStatus deck_ui_win32_input_handle_message(
    DeckUiWin32Input *input,
    DeckUiViewState *view_state,
    const DeckUiLayout *layout,
    const DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT],
    UINT message,
    WPARAM w_param,
    LPARAM l_param,
    UINT dpi,
    unsigned int modifiers,
    DeckUiWin32InputResult *result);

#ifdef __cplusplus
}
#endif

#endif
