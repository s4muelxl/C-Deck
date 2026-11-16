#include "../src/ui_presentation.h"

#include <assert.h>
#include <stdio.h>

static void test_normal_and_hover_resolution(void) {
    DeckUiViewState view;
    DeckUiPresentationState result;

    deck_ui_view_state_init(&view);
    assert(deck_ui_presentation_resolve_slot(
               &view, 1u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_NORMAL);
    assert(result.focused == 0);

    assert(deck_ui_view_set_hover(&view, 2u) == DECK_UI_VIEW_OK);
    assert(deck_ui_presentation_resolve_slot(
               &view, 2u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_HOVER);
    assert(result.focused == 0);
}

static void test_selected_and_hovered_preserves_selection(void) {
    DeckUiViewState view;
    DeckUiPresentationState result;
    DeckUiPalette palette;
    DeckUiStyle style;

    deck_ui_view_state_init(&view);
    assert(deck_ui_view_select(&view, 4u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_set_hover(&view, 4u) == DECK_UI_VIEW_OK);

    assert(deck_ui_presentation_resolve_slot(
               &view, 4u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_SELECTED);
    assert(result.focused == 1);

    deck_ui_palette_init(&palette);
    style = deck_ui_style_for_interaction(
        &palette, result.visual_state, result.focused);
    assert(style.fill == 0x18281Au);
    assert(style.border == 0x9DFF00u);
    assert(style.indicator == 0x9DFF00u);
    assert((style.flags & DECK_UI_STYLE_FOCUS_RING) != 0u);
}

static void test_pressed_takes_priority_over_selected(void) {
    DeckUiViewState view;
    DeckUiPresentationState result;

    deck_ui_view_state_init(&view);
    assert(deck_ui_view_select(&view, 7u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_pointer_press(&view, 7u) == DECK_UI_VIEW_OK);

    assert(deck_ui_presentation_resolve_slot(
               &view, 7u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_PRESSED);
    assert(result.focused == 1);
}

static void test_model_states_override_pointer_feedback(void) {
    DeckUiViewState view;
    DeckUiPresentationState result;

    deck_ui_view_state_init(&view);
    assert(deck_ui_view_pointer_press(&view, 9u) == DECK_UI_VIEW_OK);

    assert(deck_ui_presentation_resolve_slot(
               &view, 9u, DECK_UI_STATE_DISABLED, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_DISABLED);
    assert(result.focused == 0);

    assert(deck_ui_presentation_resolve_slot(
               &view, 9u, DECK_UI_STATE_ERROR, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_ERROR);
    assert(result.focused == 1);

    assert(deck_ui_presentation_resolve_slot(
               &view, 9u, DECK_UI_STATE_WARNING, &result) ==
           DECK_UI_PRESENTATION_OK);
    assert(result.visual_state == DECK_UI_STATE_WARNING);
    assert(result.focused == 1);
}

static void test_invalid_arguments_preserve_output(void) {
    DeckUiViewState view;
    DeckUiPresentationState result = { DECK_UI_STATE_ERROR, 73 };

    deck_ui_view_state_init(&view);

    assert(deck_ui_presentation_resolve_slot(
               &view, 15u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_INVALID_SLOT);
    assert(result.visual_state == DECK_UI_STATE_ERROR);
    assert(result.focused == 73);

    assert(deck_ui_presentation_resolve_slot(
               &view, 0u, DECK_UI_STATE_HOVER, &result) ==
           DECK_UI_PRESENTATION_INVALID_MODEL_STATE);
    assert(result.visual_state == DECK_UI_STATE_ERROR);
    assert(result.focused == 73);

    assert(deck_ui_presentation_resolve_slot(
               NULL, 0u, DECK_UI_STATE_NORMAL, &result) ==
           DECK_UI_PRESENTATION_INVALID_ARGUMENT);
    assert(deck_ui_presentation_resolve_slot(
               &view, 0u, DECK_UI_STATE_NORMAL, NULL) ==
           DECK_UI_PRESENTATION_INVALID_ARGUMENT);
    assert(result.visual_state == DECK_UI_STATE_ERROR);
    assert(result.focused == 73);
}

int main(void) {
    test_normal_and_hover_resolution();
    test_selected_and_hovered_preserves_selection();
    test_pressed_takes_priority_over_selected();
    test_model_states_override_pointer_feedback();
    test_invalid_arguments_preserve_output();
    puts("ui_presentation_test: all checks passed");
    return 0;
}
