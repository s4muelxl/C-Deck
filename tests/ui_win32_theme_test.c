#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui_win32_theme.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t rgb(COLORREF color) {
    return ((uint32_t)GetRValue(color) << 16u) |
           ((uint32_t)GetGValue(color) << 8u) |
           (uint32_t)GetBValue(color);
}

static void test_system_color_mapping_is_semantic_and_transactional(void) {
    const DeckUiWin32SystemColors colors = {
        RGB(0, 0, 0),
        RGB(255, 255, 255),
        RGB(16, 16, 16),
        RGB(32, 32, 32),
        RGB(180, 180, 180),
        RGB(160, 160, 160),
        RGB(0, 0, 128),
        RGB(255, 255, 0),
        RGB(0, 255, 255),
        RGB(0, 0, 0),
        RGB(255, 255, 0)
    };
    DeckUiPalette palette;
    DeckUiPalette before;
    DeckUiStyle selected;
    DeckUiStyle error;
    DeckUiStyle warning;

    deck_ui_palette_init(&palette);
    assert(deck_ui_win32_theme_palette_from_system_colors(&colors, &palette) ==
           DECK_UI_WIN32_THEME_OK);

    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_CANVAS) == rgb(colors.window));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_TEXT_PRIMARY) == rgb(colors.window_text));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_TEXT_ON_ACCENT) == rgb(colors.highlight_text));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_ACCENT) == rgb(colors.highlight));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_DANGER_BACKGROUND) == rgb(colors.window));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_DANGER) == rgb(colors.hotlight));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_WARNING_BACKGROUND) == rgb(colors.info_background));
    assert(deck_ui_palette_get(&palette, DECK_UI_COLOR_WARNING) == rgb(colors.info_text));

    selected = deck_ui_style_for_state(&palette, DECK_UI_STATE_SELECTED);
    error = deck_ui_style_for_state(&palette, DECK_UI_STATE_ERROR);
    warning = deck_ui_style_for_state(&palette, DECK_UI_STATE_WARNING);
    assert(selected.fill == rgb(colors.highlight));
    assert(selected.text == rgb(colors.highlight_text));
    assert(error.fill == rgb(colors.window));
    assert(error.text == rgb(colors.hotlight));
    assert(warning.fill == rgb(colors.info_background));
    assert(warning.text == rgb(colors.info_text));

    before = palette;
    assert(deck_ui_win32_theme_palette_from_system_colors(NULL, &palette) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(memcmp(&palette, &before, sizeof(palette)) == 0);
    assert(deck_ui_win32_theme_palette_from_system_colors(&colors, NULL) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(memcmp(&palette, &before, sizeof(palette)) == 0);
}

static void test_query_preserves_palette_when_not_active_or_unavailable(void) {
    DeckUiPalette palette;
    DeckUiPalette before;
    DeckUiWin32ThemeStatus status;

    deck_ui_palette_init(&palette);
    before = palette;
    status = deck_ui_win32_theme_query_palette(&palette);

    assert(status == DECK_UI_WIN32_THEME_OK ||
           status == DECK_UI_WIN32_THEME_NOT_ACTIVE ||
           status == DECK_UI_WIN32_THEME_SYSTEM_ERROR);
    if (status != DECK_UI_WIN32_THEME_OK) {
        assert(memcmp(&palette, &before, sizeof(palette)) == 0);
    } else {
        assert(palette.colors[DECK_UI_COLOR_TEXT_ON_ACCENT] ==
               rgb(GetSysColor(COLOR_HIGHLIGHTTEXT)));
    }
    assert(deck_ui_win32_theme_query_palette(NULL) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
}

int main(void) {
    test_system_color_mapping_is_semantic_and_transactional();
    test_query_preserves_palette_when_not_active_or_unavailable();
    puts("ui_win32_theme_test: High Contrast mapping and query OK");
    return 0;
}
