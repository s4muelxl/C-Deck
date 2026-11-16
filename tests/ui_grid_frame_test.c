#include "../src/ui_grid_frame.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void assert_rect_equal(const DeckUiRect *left, const DeckUiRect *right) {
    assert(left->x == right->x);
    assert(left->y == right->y);
    assert(left->width == right->width);
    assert(left->height == right->height);
}

static void make_wide_layout(DeckUiLayout *layout) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    assert(deck_ui_layout_compute(&input, layout) == DECK_UI_LAYOUT_OK);
}

static void test_builds_complete_renderer_frame(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiGridFrame frame;
    uint32_t index;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);

    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_OK);
    assert(frame.layout_mode == DECK_UI_LAYOUT_MODE_WIDE);
    assert(frame.slot_count == DECK_UI_GRID_SLOT_COUNT);

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        assert_rect_equal(&frame.slots[index].bounds, &layout.slots[index]);
        assert(frame.slots[index].presentation.visual_state == DECK_UI_STATE_NORMAL);
        assert(frame.slots[index].presentation.focused == (index == 0u));
        assert(frame.slots[index].style.fill == 0x191F26u);
        assert(frame.slots[index].style.border == 0x2B333Du);
        assert(((frame.slots[index].style.flags & DECK_UI_STYLE_FOCUS_RING) != 0u)
               == (index == 0u));
    }
}

static void test_selected_slot_keeps_lime_and_focus_overlay(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiGridFrame frame;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);
    assert(deck_ui_view_select(&view, 7u) == DECK_UI_VIEW_SELECTED);

    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_OK);

    assert(frame.slots[7].presentation.visual_state == DECK_UI_STATE_SELECTED);
    assert(frame.slots[7].presentation.focused == 1);
    assert(frame.slots[7].style.fill == 0x18281Au);
    assert(frame.slots[7].style.border == 0x9DFF00u);
    assert(frame.slots[7].style.indicator == 0x9DFF00u);
    assert((frame.slots[7].style.flags & DECK_UI_STYLE_FOCUS_RING) != 0u);

    assert(frame.slots[0].presentation.focused == 0);
    assert(frame.slots[0].presentation.visual_state == DECK_UI_STATE_NORMAL);
}

static void test_model_states_override_pointer_feedback(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT] = { DECK_UI_STATE_NORMAL };
    DeckUiGridFrame frame;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);

    view.focused_slot = 4u;
    view.hovered_slot = 4u;
    view.pressed_slot = 4u;
    model_states[4] = DECK_UI_STATE_DISABLED;
    model_states[5] = DECK_UI_STATE_ERROR;
    model_states[6] = DECK_UI_STATE_WARNING;

    assert(deck_ui_grid_frame_build(&layout, &view, &palette, model_states, &frame)
           == DECK_UI_GRID_FRAME_OK);

    assert(frame.slots[4].presentation.visual_state == DECK_UI_STATE_DISABLED);
    assert(frame.slots[4].presentation.focused == 0);
    assert((frame.slots[4].style.flags & DECK_UI_STYLE_FOCUS_RING) == 0u);

    assert(frame.slots[5].presentation.visual_state == DECK_UI_STATE_ERROR);
    assert(frame.slots[6].presentation.visual_state == DECK_UI_STATE_WARNING);
}

static void test_invalid_model_state_preserves_output(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT] = { DECK_UI_STATE_NORMAL };
    DeckUiGridFrame frame;
    DeckUiGridFrame before;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);
    memset(&frame, 0x5A, sizeof(frame));
    before = frame;
    model_states[3] = DECK_UI_STATE_HOVER;

    assert(deck_ui_grid_frame_build(&layout, &view, &palette, model_states, &frame)
           == DECK_UI_GRID_FRAME_INVALID_MODEL_STATE);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
}

static void test_invalid_layout_and_view_state_preserve_output(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiGridFrame frame;
    DeckUiGridFrame before;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);
    memset(&frame, 0xA5, sizeof(frame));
    before = frame;

    layout.slots[1].x = layout.slots[0].x;
    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_LAYOUT);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);

    make_wide_layout(&layout);
    view.focused_slot = DECK_UI_SLOT_NONE;
    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_VIEW_STATE);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
}

static void test_invalid_arguments(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiGridFrame frame;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);

    assert(deck_ui_grid_frame_build(NULL, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_ARGUMENT);
    assert(deck_ui_grid_frame_build(&layout, NULL, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_ARGUMENT);
    assert(deck_ui_grid_frame_build(&layout, &view, NULL, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_ARGUMENT);
    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, NULL)
           == DECK_UI_GRID_FRAME_INVALID_ARGUMENT);
}


static void test_invalid_shell_regions_reject_frame_without_mutation(void) {
    DeckUiLayout layout;
    DeckUiViewState view;
    DeckUiPalette palette;
    DeckUiGridFrame frame;
    DeckUiGridFrame before;

    make_wide_layout(&layout);
    deck_ui_view_state_init(&view);
    deck_ui_palette_init(&palette);
    memset(&frame, 0x3C, sizeof(frame));
    before = frame;

    layout.inspector.x -= 1;
    assert(deck_ui_grid_frame_build(&layout, &view, &palette, NULL, &frame)
           == DECK_UI_GRID_FRAME_INVALID_LAYOUT);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
}

int main(void) {
    test_builds_complete_renderer_frame();
    test_selected_slot_keeps_lime_and_focus_overlay();
    test_model_states_override_pointer_feedback();
    test_invalid_model_state_preserves_output();
    test_invalid_layout_and_view_state_preserve_output();
    test_invalid_shell_regions_reject_frame_without_mutation();
    test_invalid_arguments();
    puts("ui_grid_frame_test: all checks passed");
    return 0;
}
