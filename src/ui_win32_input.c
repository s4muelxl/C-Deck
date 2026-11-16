#include "ui_win32_input.h"

#include <string.h>

#define DECK_UI_WIN32_MODIFIER_MASK \
    (DECK_UI_WIN32_MODIFIER_CONTROL | DECK_UI_WIN32_MODIFIER_META)

static int valid_slot_or_none(uint32_t slot) {
    return slot == DECK_UI_SLOT_NONE || slot < DECK_UI_GRID_SLOT_COUNT;
}

static int view_state_is_valid(const DeckUiViewState *view) {
    return view &&
        view->focused_slot < DECK_UI_GRID_SLOT_COUNT &&
        valid_slot_or_none(view->selected_slot) &&
        valid_slot_or_none(view->hovered_slot) &&
        valid_slot_or_none(view->pressed_slot);
}

static int input_state_is_valid(const DeckUiWin32Input *input) {
    if (!input) return 0;

    switch (input->press_source) {
        case DECK_UI_WIN32_PRESS_NONE:
            return input->mouse_pressed_slot == DECK_UI_SLOT_NONE &&
                   input->keyboard_pressed_slot == DECK_UI_SLOT_NONE;
        case DECK_UI_WIN32_PRESS_MOUSE:
            return input->mouse_pressed_slot < DECK_UI_GRID_SLOT_COUNT &&
                   input->keyboard_pressed_slot == DECK_UI_SLOT_NONE;
        case DECK_UI_WIN32_PRESS_KEYBOARD:
            return input->keyboard_pressed_slot < DECK_UI_GRID_SLOT_COUNT &&
                   input->mouse_pressed_slot == DECK_UI_SLOT_NONE;
        default:
            return 0;
    }
}

static int input_matches_view(const DeckUiWin32Input *input,
                              const DeckUiViewState *view) {
    switch (input->press_source) {
        case DECK_UI_WIN32_PRESS_NONE:
            return view->pressed_slot == DECK_UI_SLOT_NONE;
        case DECK_UI_WIN32_PRESS_MOUSE:
            return view->pressed_slot == DECK_UI_SLOT_NONE ||
                   view->pressed_slot == input->mouse_pressed_slot;
        case DECK_UI_WIN32_PRESS_KEYBOARD:
            return view->pressed_slot == DECK_UI_SLOT_NONE ||
                   view->pressed_slot == input->keyboard_pressed_slot;
        default:
            return 0;
    }
}

static void clear_press_source(DeckUiWin32Input *input) {
    input->mouse_pressed_slot = DECK_UI_SLOT_NONE;
    input->keyboard_pressed_slot = DECK_UI_SLOT_NONE;
    input->press_source = DECK_UI_WIN32_PRESS_NONE;
}

static void cancel_active_press(DeckUiWin32Input *input,
                                DeckUiViewState *view) {
    const uint32_t hovered = view->hovered_slot;

    deck_ui_view_clear_transient(view);
    if (hovered != DECK_UI_SLOT_NONE) {
        (void)deck_ui_view_set_hover(view, hovered);
    }
    clear_press_source(input);
}

static int slot_is_focusable(uint16_t mask, uint32_t slot) {
    return slot < DECK_UI_GRID_SLOT_COUNT &&
           (mask & (uint16_t)(1u << slot)) != 0u;
}

static DeckUiWin32InputStatus reconcile_focus_if_needed(
    DeckUiWin32Input *input,
    DeckUiViewState *view,
    uint16_t focusable_mask) {
    DeckUiViewStatus status;

    if (focusable_mask == 0u) {
        return DECK_UI_WIN32_INPUT_NO_FOCUSABLE_SLOT;
    }
    if (slot_is_focusable(focusable_mask, view->focused_slot)) {
        return DECK_UI_WIN32_INPUT_OK;
    }

    status = deck_ui_view_reconcile_focus(view, focusable_mask);
    if (status != DECK_UI_VIEW_OK) {
        return status == DECK_UI_VIEW_NO_FOCUSABLE_SLOT
            ? DECK_UI_WIN32_INPUT_NO_FOCUSABLE_SLOT
            : DECK_UI_WIN32_INPUT_INVALID_STATE;
    }
    clear_press_source(input);
    return DECK_UI_WIN32_INPUT_OK;
}

static int32_t signed_low_word(LPARAM value) {
    return (int16_t)(uint16_t)(uintptr_t)value;
}

static int32_t signed_high_word(LPARAM value) {
    return (int16_t)(uint16_t)(((uintptr_t)value >> 16u) & 0xFFFFu);
}

static DeckUiWin32InputStatus hit_test_message_point(
    const DeckUiLayout *layout,
    LPARAM l_param,
    UINT dpi,
    uint32_t *slot) {
    const int32_t x_dip = deck_ui_px_to_dip(signed_low_word(l_param), dpi);
    const int32_t y_dip = deck_ui_px_to_dip(signed_high_word(l_param), dpi);
    DeckUiHitTestStatus status;

    status = deck_ui_grid_hit_test(layout, x_dip, y_dip, slot);
    if (status == DECK_UI_HIT_TEST_INVALID_LAYOUT) {
        return DECK_UI_WIN32_INPUT_INVALID_LAYOUT;
    }
    if (status == DECK_UI_HIT_TEST_INVALID_ARGUMENT) {
        return DECK_UI_WIN32_INPUT_INVALID_ARGUMENT;
    }
    if (status != DECK_UI_HIT_TEST_HIT) *slot = DECK_UI_SLOT_NONE;
    return DECK_UI_WIN32_INPUT_OK;
}

static DeckUiWin32InputStatus return_unhandled(
    DeckUiWin32InputResult *result) {
    DeckUiWin32InputResult empty;

    memset(&empty, 0, sizeof(empty));
    empty.selected_slot = DECK_UI_SLOT_NONE;
    *result = empty;
    return DECK_UI_WIN32_INPUT_UNHANDLED;
}

static int key_is_repeated(LPARAM l_param) {
    return (((uintptr_t)l_param >> 30u) & 1u) != 0u;
}

static void finish_result(const DeckUiViewState *before,
                          const DeckUiViewState *after,
                          DeckUiWin32InputResult *result) {
    result->focus_changed = before->focused_slot != after->focused_slot;
    result->hover_changed = before->hovered_slot != after->hovered_slot;
    result->pressed_changed = before->pressed_slot != after->pressed_slot;
    result->selection_changed =
        before->selected_slot != after->selected_slot &&
        after->selected_slot != DECK_UI_SLOT_NONE;
    result->selected_slot = result->selection_changed
        ? after->selected_slot : DECK_UI_SLOT_NONE;
    result->invalidate = result->focus_changed || result->hover_changed ||
        result->pressed_changed || result->selection_changed;
}

void deck_ui_win32_input_init(DeckUiWin32Input *input) {
    if (!input) return;
    memset(input, 0, sizeof(*input));
    input->mouse_pressed_slot = DECK_UI_SLOT_NONE;
    input->keyboard_pressed_slot = DECK_UI_SLOT_NONE;
    input->press_source = DECK_UI_WIN32_PRESS_NONE;
}

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
    DeckUiWin32InputResult *result) {
    DeckUiWin32Input next_input;
    DeckUiViewState next_view;
    DeckUiViewState before_view;
    DeckUiWin32InputResult next_result;
    uint16_t focusable_mask = 0u;
    DeckUiGridFrameStatus mask_status;
    DeckUiWin32InputStatus status = DECK_UI_WIN32_INPUT_OK;
    uint32_t hit_slot = DECK_UI_SLOT_NONE;

    if (!input || !view_state || !layout || !result ||
        (dpi != 0u && (dpi < 48u || dpi > 768u)) ||
        (modifiers & ~DECK_UI_WIN32_MODIFIER_MASK) != 0u) {
        return DECK_UI_WIN32_INPUT_INVALID_ARGUMENT;
    }
    if (!input_state_is_valid(input) || !view_state_is_valid(view_state) ||
        !input_matches_view(input, view_state)) {
        return DECK_UI_WIN32_INPUT_INVALID_STATE;
    }

    mask_status = deck_ui_grid_focusable_mask_from_states(
        model_states, &focusable_mask);
    if (mask_status != DECK_UI_GRID_FRAME_OK) {
        return DECK_UI_WIN32_INPUT_INVALID_MODEL_STATE;
    }

    next_input = *input;
    next_view = *view_state;
    before_view = *view_state;
    memset(&next_result, 0, sizeof(next_result));
    next_result.selected_slot = DECK_UI_SLOT_NONE;

    switch (message) {
        case WM_MOUSEMOVE:
            status = hit_test_message_point(layout, l_param, dpi, &hit_slot);
            if (status != DECK_UI_WIN32_INPUT_OK) return status;
            next_result.handled = 1;
            next_result.request_mouse_leave_tracking = 1;
            if (next_input.press_source == DECK_UI_WIN32_PRESS_MOUSE) {
                if (deck_ui_view_pointer_drag(
                        &next_view, next_input.mouse_pressed_slot, hit_slot)
                    != DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
            } else if (deck_ui_view_set_hover(&next_view, hit_slot) !=
                       DECK_UI_VIEW_OK) {
                return DECK_UI_WIN32_INPUT_INVALID_STATE;
            }
            break;

        case WM_MOUSELEAVE:
            next_result.handled = 1;
            if (next_input.press_source == DECK_UI_WIN32_PRESS_MOUSE) {
                if (deck_ui_view_pointer_drag(
                        &next_view, next_input.mouse_pressed_slot,
                        DECK_UI_SLOT_NONE) != DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
            } else if (deck_ui_view_set_hover(
                           &next_view, DECK_UI_SLOT_NONE) != DECK_UI_VIEW_OK) {
                return DECK_UI_WIN32_INPUT_INVALID_STATE;
            }
            break;

        case WM_LBUTTONDOWN:
            status = hit_test_message_point(layout, l_param, dpi, &hit_slot);
            if (status != DECK_UI_WIN32_INPUT_OK) return status;
            if (hit_slot == DECK_UI_SLOT_NONE) {
                return return_unhandled(result);
            }
            next_result.handled = 1;
            if (next_input.press_source != DECK_UI_WIN32_PRESS_NONE) break;
            if (deck_ui_view_set_hover(&next_view, hit_slot) !=
                DECK_UI_VIEW_OK) {
                return DECK_UI_WIN32_INPUT_INVALID_STATE;
            }
            if (slot_is_focusable(focusable_mask, hit_slot)) {
                if (deck_ui_view_pointer_press(&next_view, hit_slot) !=
                    DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                next_input.mouse_pressed_slot = hit_slot;
                next_input.press_source = DECK_UI_WIN32_PRESS_MOUSE;
                next_result.capture_mouse = 1;
            }
            break;

        case WM_LBUTTONUP:
            if (next_input.press_source != DECK_UI_WIN32_PRESS_MOUSE) {
                return return_unhandled(result);
            }
            status = hit_test_message_point(layout, l_param, dpi, &hit_slot);
            if (status != DECK_UI_WIN32_INPUT_OK) return status;
            next_result.handled = 1;
            next_result.release_mouse_capture = 1;
            if (hit_slot == next_input.mouse_pressed_slot &&
                slot_is_focusable(focusable_mask, hit_slot)) {
                if (deck_ui_view_pointer_drag(
                        &next_view, next_input.mouse_pressed_slot, hit_slot)
                    != DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                if (deck_ui_view_pointer_release(&next_view, hit_slot) !=
                    DECK_UI_VIEW_SELECTED) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
            } else {
                if (hit_slot == next_input.mouse_pressed_slot) {
                    if (deck_ui_view_pointer_drag(
                            &next_view, next_input.mouse_pressed_slot,
                            DECK_UI_SLOT_NONE) != DECK_UI_VIEW_OK) {
                        return DECK_UI_WIN32_INPUT_INVALID_STATE;
                    }
                    if (hit_slot != DECK_UI_SLOT_NONE) {
                        (void)deck_ui_view_set_hover(&next_view, hit_slot);
                    }
                } else if (deck_ui_view_pointer_drag(
                               &next_view, next_input.mouse_pressed_slot,
                               hit_slot) != DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                (void)deck_ui_view_pointer_release(&next_view, hit_slot);
            }
            clear_press_source(&next_input);
            break;

        case WM_CAPTURECHANGED:
            if (next_input.press_source != DECK_UI_WIN32_PRESS_MOUSE) {
                return return_unhandled(result);
            }
            next_result.handled = 1;
            if (deck_ui_view_pointer_drag(
                    &next_view, next_input.mouse_pressed_slot,
                    DECK_UI_SLOT_NONE) != DECK_UI_VIEW_OK) {
                return DECK_UI_WIN32_INPUT_INVALID_STATE;
            }
            (void)deck_ui_view_pointer_release(&next_view, DECK_UI_SLOT_NONE);
            clear_press_source(&next_input);
            break;

        case WM_KILLFOCUS:
            next_result.handled = 1;
            if (next_input.press_source == DECK_UI_WIN32_PRESS_MOUSE) {
                next_result.release_mouse_capture = 1;
            }
            deck_ui_view_clear_transient(&next_view);
            clear_press_source(&next_input);
            break;

        case WM_CANCELMODE:
            /*
             * Windows sends WM_CANCELMODE when the current interaction mode
             * must end (for example, before a menu takes over). Clear both
             * mouse and keyboard presses; the window procedure performs the
             * actual ReleaseCapture when requested below.
             */
            next_result.handled = 1;
            if (next_input.press_source == DECK_UI_WIN32_PRESS_MOUSE) {
                next_result.release_mouse_capture = 1;
            }
            deck_ui_view_clear_transient(&next_view);
            clear_press_source(&next_input);
            break;

        case WM_KEYDOWN: {
            DeckUiDirection direction;
            int is_direction = 1;

            switch (w_param) {
                case VK_LEFT: direction = DECK_UI_DIRECTION_LEFT; break;
                case VK_RIGHT: direction = DECK_UI_DIRECTION_RIGHT; break;
                case VK_UP: direction = DECK_UI_DIRECTION_UP; break;
                case VK_DOWN: direction = DECK_UI_DIRECTION_DOWN; break;
                default: is_direction = 0; direction = DECK_UI_DIRECTION_LEFT; break;
            }

            if (is_direction || w_param == VK_HOME || w_param == VK_END) {
                DeckUiViewStatus view_status;

                next_result.handled = 1;
                if (next_input.press_source == DECK_UI_WIN32_PRESS_MOUSE) {
                    break;
                }
                if (next_input.press_source == DECK_UI_WIN32_PRESS_KEYBOARD) {
                    cancel_active_press(&next_input, &next_view);
                }
                status = reconcile_focus_if_needed(
                    &next_input, &next_view, focusable_mask);
                if (status != DECK_UI_WIN32_INPUT_OK) break;

                if (is_direction) {
                    view_status = deck_ui_view_move_focusable(
                        &next_view, direction, focusable_mask);
                } else {
                    const int to_end = w_param == VK_END;
                    const int entire_grid =
                        (modifiers & DECK_UI_WIN32_MODIFIER_MASK) != 0u;
                    view_status = deck_ui_view_focus_boundary_focusable(
                        &next_view, to_end, entire_grid, focusable_mask);
                }
                if (view_status == DECK_UI_VIEW_NO_FOCUSABLE_SLOT) {
                    status = DECK_UI_WIN32_INPUT_NO_FOCUSABLE_SLOT;
                } else if (view_status != DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                break;
            }

            if (w_param == VK_RETURN) {
                next_result.handled = 1;
                if (key_is_repeated(l_param) ||
                    next_input.press_source != DECK_UI_WIN32_PRESS_NONE) {
                    break;
                }
                status = reconcile_focus_if_needed(
                    &next_input, &next_view, focusable_mask);
                if (status != DECK_UI_WIN32_INPUT_OK) break;
                if (deck_ui_view_select(&next_view, next_view.focused_slot) !=
                    DECK_UI_VIEW_SELECTED) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                break;
            }

            if (w_param == VK_SPACE) {
                next_result.handled = 1;
                if (key_is_repeated(l_param) ||
                    next_input.press_source != DECK_UI_WIN32_PRESS_NONE) {
                    break;
                }
                status = reconcile_focus_if_needed(
                    &next_input, &next_view, focusable_mask);
                if (status != DECK_UI_WIN32_INPUT_OK) break;
                if (deck_ui_view_pointer_press(
                        &next_view, next_view.focused_slot) !=
                    DECK_UI_VIEW_OK) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                next_input.keyboard_pressed_slot = next_view.focused_slot;
                next_input.press_source = DECK_UI_WIN32_PRESS_KEYBOARD;
                break;
            }

            return return_unhandled(result);
        }

        case WM_KEYUP:
            if (w_param != VK_SPACE ||
                next_input.press_source != DECK_UI_WIN32_PRESS_KEYBOARD) {
                return return_unhandled(result);
            }
            next_result.handled = 1;
            if (next_view.pressed_slot == next_input.keyboard_pressed_slot &&
                slot_is_focusable(focusable_mask,
                                  next_input.keyboard_pressed_slot)) {
                if (deck_ui_view_pointer_release(
                        &next_view, next_input.keyboard_pressed_slot) !=
                    DECK_UI_VIEW_SELECTED) {
                    return DECK_UI_WIN32_INPUT_INVALID_STATE;
                }
                (void)deck_ui_view_set_hover(&next_view, DECK_UI_SLOT_NONE);
            } else {
                cancel_active_press(&next_input, &next_view);
            }
            clear_press_source(&next_input);
            break;

        default:
            return return_unhandled(result);
    }

    finish_result(&before_view, &next_view, &next_result);
    *input = next_input;
    *view_state = next_view;
    *result = next_result;
    return status;
}
