#include "../src/ui_theme.h"

#include <assert.h>
#include <stdio.h>

static void test_palette_tokens(void) {
    DeckUiPalette palette;

    deck_ui_palette_init(&palette);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_CANVAS) == 0x0B0D10u);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_SURFACE_1) == 0x12161Bu);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_TEXT_PRIMARY) == 0xF3F5F7u);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_ACCENT) == 0x9DFF00u);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_DANGER) == 0xF06B6Bu);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_WARNING) == 0xE6B85Cu);
    assert(deck_ui_palette_get(NULL, DECK_UI_COLOR_CANVAS) == 0u);
    assert(deck_ui_palette_get(&palette, (DeckUiColorRole)-1) == 0u);
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_COUNT) == 0u);
}

static void test_component_states(void) {
    DeckUiPalette palette;
    DeckUiStyle normal;
    DeckUiStyle hover;
    DeckUiStyle pressed;
    DeckUiStyle selected;
    DeckUiStyle disabled;
    DeckUiStyle focused;
    DeckUiStyle error;
    DeckUiStyle warning;

    deck_ui_palette_init(&palette);
    normal = deck_ui_style_for_state(&palette, DECK_UI_STATE_NORMAL);
    hover = deck_ui_style_for_state(&palette, DECK_UI_STATE_HOVER);
    pressed = deck_ui_style_for_state(&palette, DECK_UI_STATE_PRESSED);
    selected = deck_ui_style_for_state(&palette, DECK_UI_STATE_SELECTED);
    disabled = deck_ui_style_for_state(&palette, DECK_UI_STATE_DISABLED);
    focused = deck_ui_style_for_state(&palette, DECK_UI_STATE_FOCUSED);
    error = deck_ui_style_for_state(&palette, DECK_UI_STATE_ERROR);
    warning = deck_ui_style_for_state(&palette, DECK_UI_STATE_WARNING);

    assert(normal.fill == 0x191F26u && normal.border == 0x2B333Du);
    assert(hover.fill == 0x222A33u && hover.border == 0x46515Eu);
    assert(pressed.fill == 0x0B0D10u);
    assert(selected.fill == 0x18281Au);
    assert(selected.border == 0x9DFF00u && selected.indicator == 0x9DFF00u);
    assert(disabled.text == 0x69737Fu);
    assert(disabled.border != 0x9DFF00u);
    assert(focused.border == 0xFFFFFFu);
    assert((focused.flags & DECK_UI_STYLE_FOCUS_RING) != 0u);
    assert(error.text == 0xF06B6Bu && error.border == 0xF06B6Bu);
    assert(warning.text == 0xE6B85Cu && warning.border == 0xE6B85Cu);
}

static void test_invalid_state_falls_back_safely(void) {
    DeckUiPalette palette;
    DeckUiStyle normal;
    DeckUiStyle invalid;
    DeckUiStyle empty;

    deck_ui_palette_init(&palette);
    normal = deck_ui_style_for_state(&palette, DECK_UI_STATE_NORMAL);
    invalid = deck_ui_style_for_state(&palette, (DeckUiVisualState)999);
    empty = deck_ui_style_for_state(NULL, DECK_UI_STATE_SELECTED);

    assert(invalid.fill == normal.fill);
    assert(invalid.text == normal.text);
    assert(invalid.border == normal.border);
    assert(invalid.indicator == normal.indicator);
    assert(empty.fill == 0u && empty.text == 0u && empty.border == 0u);
    assert(empty.flags == DECK_UI_STYLE_NONE);
}

int main(void) {
    test_palette_tokens();
    test_component_states();
    test_invalid_state_falls_back_safely();
    puts("ui_theme_test: all checks passed");
    return 0;
}
