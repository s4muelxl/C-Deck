#include "../src/ui_theme.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static double srgb_channel_to_linear(uint32_t channel) {
    const double value = (double)channel / 255.0;
    return value <= 0.04045 ? value / 12.92 : pow((value + 0.055) / 1.055, 2.4);
}

static double relative_luminance(uint32_t rgb) {
    const double red = srgb_channel_to_linear((rgb >> 16) & 0xFFu);
    const double green = srgb_channel_to_linear((rgb >> 8) & 0xFFu);
    const double blue = srgb_channel_to_linear(rgb & 0xFFu);
    return 0.2126 * red + 0.7152 * green + 0.0722 * blue;
}

static double contrast_ratio(uint32_t foreground, uint32_t background) {
    const double foreground_luminance = relative_luminance(foreground);
    const double background_luminance = relative_luminance(background);
    const double lighter = foreground_luminance > background_luminance
                               ? foreground_luminance : background_luminance;
    const double darker = foreground_luminance > background_luminance
                              ? background_luminance : foreground_luminance;
    return (lighter + 0.05) / (darker + 0.05);
}

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

static void test_wcag_palette_contrast(void) {
    static const DeckUiColorRole backgrounds[] = {
        DECK_UI_COLOR_CANVAS,
        DECK_UI_COLOR_SURFACE_1,
        DECK_UI_COLOR_SURFACE_2,
        DECK_UI_COLOR_SURFACE_3,
        DECK_UI_COLOR_ACCENT_BACKGROUND
    };
    static const DeckUiColorRole normal_text[] = {
        DECK_UI_COLOR_TEXT_PRIMARY,
        DECK_UI_COLOR_TEXT_SECONDARY
    };
    static const DeckUiColorRole semantic_indicators[] = {
        DECK_UI_COLOR_ACCENT,
        DECK_UI_COLOR_DANGER,
        DECK_UI_COLOR_WARNING,
        DECK_UI_COLOR_FOCUS
    };
    DeckUiPalette palette;
    size_t background_index;

    deck_ui_palette_init(&palette);

    for (background_index = 0u;
         background_index < sizeof(backgrounds) / sizeof(backgrounds[0]);
         ++background_index) {
        const uint32_t background = deck_ui_palette_get(&palette, backgrounds[background_index]);
        size_t text_index;
        size_t indicator_index;

        for (text_index = 0u;
             text_index < sizeof(normal_text) / sizeof(normal_text[0]);
             ++text_index) {
            const uint32_t foreground = deck_ui_palette_get(&palette, normal_text[text_index]);
            assert(contrast_ratio(foreground, background) >= 4.5);
        }

        assert(contrast_ratio(deck_ui_palette_get(&palette, DECK_UI_COLOR_TEXT_DISABLED),
                              background) >= 3.0);

        for (indicator_index = 0u;
             indicator_index < sizeof(semantic_indicators) / sizeof(semantic_indicators[0]);
             ++indicator_index) {
            const uint32_t foreground =
                deck_ui_palette_get(&palette, semantic_indicators[indicator_index]);
            assert(contrast_ratio(foreground, background) >= 3.0);
        }
    }
}

int main(void) {
    test_palette_tokens();
    test_component_states();
    test_invalid_state_falls_back_safely();
    test_wcag_palette_contrast();
    puts("ui_theme_test: all checks passed");
    return 0;
}
