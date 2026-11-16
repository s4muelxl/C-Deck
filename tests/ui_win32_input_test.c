#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui_win32_input.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void prepare(DeckUiLayout *layout,
                    DeckUiWin32Input *input,
                    DeckUiViewState *view) {
    const DeckUiLayoutInput geometry = { 1200, 760, 0 };

    assert(deck_ui_layout_compute(&geometry, layout) == DECK_UI_LAYOUT_OK);
    deck_ui_win32_input_init(input);
    deck_ui_view_state_init(view);
}

static LPARAM point_for_slot(const DeckUiLayout *layout,
                             uint32_t slot,
                             UINT dpi) {
    const DeckUiRect *bounds = &layout->slots[slot];
    const int32_t x_dip = bounds->x + bounds->width / 2;
    const int32_t y_dip = bounds->y + bounds->height / 2;
    const int32_t x_px = deck_ui_dip_to_px(x_dip, dpi);
    const int32_t y_px = deck_ui_dip_to_px(y_dip, dpi);

    return MAKELPARAM((WORD)(int16_t)x_px, (WORD)(int16_t)y_px);
}

static DeckUiWin32InputStatus send_message(
    DeckUiWin32Input *input,
    DeckUiViewState *view,
    const DeckUiLayout *layout,
    const DeckUiVisualState states[DECK_UI_GRID_SLOT_COUNT],
    UINT message,
    WPARAM w_param,
    LPARAM l_param,
    UINT dpi,
    unsigned int modifiers,
    DeckUiWin32InputResult *result) {
    return deck_ui_win32_input_handle_message(
        input, view, layout, states, message, w_param, l_param,
        dpi, modifiers, result);
}

static void test_mouse_drag_feedback_and_click_selection(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiWin32InputResult result;
    LPARAM slot7;
    LPARAM slot8;

    prepare(&layout, &input, &view);
    slot7 = point_for_slot(&layout, 7u, 144u);
    slot8 = point_for_slot(&layout, 8u, 144u);

    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONDOWN,
                        MK_LBUTTON, slot7, 144u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(result.handled && result.capture_mouse && result.invalidate);
    assert(view.pressed_slot == 7u);
    assert(view.selected_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_MOUSE);

    assert(send_message(&input, &view, &layout, NULL, WM_MOUSEMOVE,
                        MK_LBUTTON, slot8, 144u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.hovered_slot == 8u);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);

    assert(send_message(&input, &view, &layout, NULL, WM_MOUSEMOVE,
                        MK_LBUTTON, slot7, 144u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.hovered_slot == 7u);
    assert(view.pressed_slot == 7u);

    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONUP,
                        0u, slot7, 144u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(result.release_mouse_capture);
    assert(result.selection_changed && result.selected_slot == 7u);
    assert(view.selected_slot == 7u);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_NONE);
}

static void test_mouse_release_elsewhere_cancels_selection(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiWin32InputResult result;

    prepare(&layout, &input, &view);
    assert(deck_ui_view_select(&view, 2u) == DECK_UI_VIEW_SELECTED);

    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONDOWN,
                        MK_LBUTTON, point_for_slot(&layout, 4u, 96u),
                        96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONUP,
                        0u, point_for_slot(&layout, 5u, 96u),
                        96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(!result.selection_changed);
    assert(view.selected_slot == 2u);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(view.hovered_slot == 5u);
}

static void test_disabled_slot_ignores_pointer_press(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiVisualState states[DECK_UI_GRID_SLOT_COUNT] = {
        DECK_UI_STATE_NORMAL
    };
    DeckUiWin32InputResult result;

    prepare(&layout, &input, &view);
    states[1] = DECK_UI_STATE_DISABLED;

    assert(send_message(&input, &view, &layout, states, WM_LBUTTONDOWN,
                        MK_LBUTTON, point_for_slot(&layout, 1u, 96u),
                        96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(result.handled);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(view.selected_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_NONE);
}

static void test_mouse_leave_capture_loss_and_focus_loss(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiWin32InputResult result;
    LPARAM slot3;

    prepare(&layout, &input, &view);
    slot3 = point_for_slot(&layout, 3u, 96u);
    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONDOWN,
                        MK_LBUTTON, slot3, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);

    assert(send_message(&input, &view, &layout, NULL, WM_MOUSELEAVE,
                        0u, 0, 96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(view.hovered_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_MOUSE);

    assert(send_message(&input, &view, &layout, NULL, WM_MOUSEMOVE,
                        MK_LBUTTON, slot3, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == 3u);

    assert(send_message(&input, &view, &layout, NULL, WM_CAPTURECHANGED,
                        0u, 0, 96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_NONE);

    assert(send_message(&input, &view, &layout, NULL, WM_KEYDOWN,
                        VK_SPACE, 0, 96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == view.focused_slot);
    assert(input.press_source == DECK_UI_WIN32_PRESS_KEYBOARD);
    assert(send_message(&input, &view, &layout, NULL, WM_KILLFOCUS,
                        0u, 0, 96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(view.hovered_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_NONE);
    assert(send_message(&input, &view, &layout, NULL, WM_KEYUP,
                        VK_SPACE, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_UNHANDLED);
}

static void test_keyboard_navigation_skips_disabled_and_uses_boundaries(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiVisualState states[DECK_UI_GRID_SLOT_COUNT] = {
        DECK_UI_STATE_NORMAL
    };
    DeckUiWin32InputResult result;

    prepare(&layout, &input, &view);
    states[1] = DECK_UI_STATE_DISABLED;
    states[2] = DECK_UI_STATE_DISABLED;

    assert(send_message(&input, &view, &layout, states, WM_KEYDOWN,
                        VK_RIGHT, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(result.handled && result.focus_changed);
    assert(view.focused_slot == 3u);
    assert(view.selected_slot == DECK_UI_SLOT_NONE);

    assert(send_message(&input, &view, &layout, states, WM_KEYDOWN,
                        VK_HOME, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.focused_slot == 0u);

    assert(send_message(&input, &view, &layout, states, WM_KEYDOWN,
                        VK_END, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.focused_slot == 4u);

    assert(send_message(&input, &view, &layout, states, WM_KEYDOWN,
                        VK_END, 0, 96u, DECK_UI_WIN32_MODIFIER_CONTROL,
                        &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.focused_slot == 14u);
}

static void test_enter_and_space_select_without_execution(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiWin32InputResult result;
    LPARAM repeat = (LPARAM)((uintptr_t)1u << 30u);

    prepare(&layout, &input, &view);
    assert(deck_ui_view_focus(&view, 9u) == DECK_UI_VIEW_OK);

    assert(send_message(&input, &view, &layout, NULL, WM_KEYDOWN,
                        VK_RETURN, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(result.selection_changed && result.selected_slot == 9u);
    assert(view.selected_slot == 9u);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);

    assert(deck_ui_view_focus(&view, 6u) == DECK_UI_VIEW_OK);
    assert(send_message(&input, &view, &layout, NULL, WM_KEYDOWN,
                        VK_SPACE, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == 6u);
    assert(view.selected_slot == 9u);

    assert(send_message(&input, &view, &layout, NULL, WM_KEYDOWN,
                        VK_SPACE, repeat, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(view.pressed_slot == 6u);
    assert(view.selected_slot == 9u);

    assert(send_message(&input, &view, &layout, NULL, WM_KEYUP,
                        VK_SPACE, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_OK);
    assert(result.selection_changed && result.selected_slot == 6u);
    assert(view.selected_slot == 6u);
    assert(view.pressed_slot == DECK_UI_SLOT_NONE);
    assert(view.hovered_slot == DECK_UI_SLOT_NONE);
    assert(input.press_source == DECK_UI_WIN32_PRESS_NONE);
}

static void test_gap_negative_coordinates_and_dpi_scaling(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiWin32InputResult result;
    const DeckUiRect *first;

    prepare(&layout, &input, &view);
    first = &layout.slots[0];

    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONDOWN,
                        MK_LBUTTON,
                        MAKELPARAM((WORD)(int16_t)(first->x + first->width +
                            layout.slot_gap_dip / 2),
                            (WORD)(int16_t)(first->y + first->height / 2)),
                        96u, 0u, &result) == DECK_UI_WIN32_INPUT_UNHANDLED);
    assert(view.selected_slot == DECK_UI_SLOT_NONE);

    assert(send_message(&input, &view, &layout, NULL, WM_MOUSEMOVE,
                        0u, MAKELPARAM((WORD)(int16_t)-5,
                                      (WORD)(int16_t)-7),
                        96u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.hovered_slot == DECK_UI_SLOT_NONE);

    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONDOWN,
                        MK_LBUTTON, point_for_slot(&layout, 14u, 192u),
                        192u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(send_message(&input, &view, &layout, NULL, WM_LBUTTONUP,
                        0u, point_for_slot(&layout, 14u, 192u),
                        192u, 0u, &result) == DECK_UI_WIN32_INPUT_OK);
    assert(view.selected_slot == 14u);
}

static void test_invalid_layout_model_and_state_are_transactional(void) {
    DeckUiLayout layout;
    DeckUiLayout invalid_layout;
    DeckUiWin32Input input;
    DeckUiWin32Input before_input;
    DeckUiViewState view;
    DeckUiViewState before_view;
    DeckUiVisualState invalid_states[DECK_UI_GRID_SLOT_COUNT] = {
        DECK_UI_STATE_NORMAL
    };
    DeckUiWin32InputResult result;
    DeckUiWin32InputResult before_result;

    prepare(&layout, &input, &view);
    invalid_layout = layout;
    invalid_layout.slots[1].x = invalid_layout.slots[0].x;
    memset(&result, 0x5A, sizeof(result));
    before_result = result;
    before_input = input;
    before_view = view;

    assert(send_message(&input, &view, &invalid_layout, NULL, WM_MOUSEMOVE,
                        0u, point_for_slot(&layout, 0u, 96u),
                        96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_INVALID_LAYOUT);
    assert(memcmp(&input, &before_input, sizeof(input)) == 0);
    assert(memcmp(&view, &before_view, sizeof(view)) == 0);
    assert(memcmp(&result, &before_result, sizeof(result)) == 0);

    invalid_states[4] = (DeckUiVisualState)99;
    assert(send_message(&input, &view, &layout, invalid_states, WM_KEYDOWN,
                        VK_RIGHT, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_INVALID_MODEL_STATE);
    assert(memcmp(&input, &before_input, sizeof(input)) == 0);
    assert(memcmp(&view, &before_view, sizeof(view)) == 0);
    assert(memcmp(&result, &before_result, sizeof(result)) == 0);

    view.pressed_slot = 4u;
    before_view = view;
    assert(send_message(&input, &view, &layout, NULL, WM_KEYDOWN,
                        VK_RIGHT, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_INVALID_STATE);
    assert(memcmp(&view, &before_view, sizeof(view)) == 0);
}

static void test_no_focusable_slot_is_reported_without_mutation(void) {
    DeckUiLayout layout;
    DeckUiWin32Input input;
    DeckUiViewState view;
    DeckUiVisualState states[DECK_UI_GRID_SLOT_COUNT];
    DeckUiViewState before_view;
    DeckUiWin32InputResult result;
    uint32_t index;

    prepare(&layout, &input, &view);
    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        states[index] = DECK_UI_STATE_DISABLED;
    }
    before_view = view;

    assert(send_message(&input, &view, &layout, states, WM_KEYDOWN,
                        VK_RIGHT, 0, 96u, 0u, &result) ==
           DECK_UI_WIN32_INPUT_NO_FOCUSABLE_SLOT);
    assert(result.handled);
    assert(memcmp(&view, &before_view, sizeof(view)) == 0);
}

int main(void) {
    test_mouse_drag_feedback_and_click_selection();
    test_mouse_release_elsewhere_cancels_selection();
    test_disabled_slot_ignores_pointer_press();
    test_mouse_leave_capture_loss_and_focus_loss();
    test_keyboard_navigation_skips_disabled_and_uses_boundaries();
    test_enter_and_space_select_without_execution();
    test_gap_negative_coordinates_and_dpi_scaling();
    test_invalid_layout_model_and_state_are_transactional();
    test_no_focusable_slot_is_reported_without_mutation();
    puts("ui_win32_input_test: all checks passed");
    return 0;
}
