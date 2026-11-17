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

static DeckUiWin32SystemColors sample_system_colors(void) {
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
    return colors;
}

static int palettes_equal(const DeckUiPalette *left,
                          const DeckUiPalette *right) {
    return memcmp(left, right, sizeof(*left)) == 0;
}

static void test_system_color_mapping_is_semantic_and_transactional(void) {
    const DeckUiWin32SystemColors colors = sample_system_colors();
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
    assert(palettes_equal(&palette, &before));
    assert(deck_ui_win32_theme_palette_from_system_colors(&colors, NULL) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&palette, &before));
}

static void test_palette_resolve_restores_normal_palette_when_disabled(void) {
    const DeckUiWin32SystemColors colors = sample_system_colors();
    DeckUiPalette normal;
    DeckUiPalette high_contrast;
    DeckUiPalette resolved;

    deck_ui_palette_init(&normal);
    assert(deck_ui_win32_theme_palette_from_system_colors(
               &colors, &high_contrast) == DECK_UI_WIN32_THEME_OK);

    /* Simulate a live palette that was previously switched to High Contrast. */
    resolved = high_contrast;
    assert(deck_ui_win32_theme_palette_resolve(
               &normal, NULL, 0, &resolved) == DECK_UI_WIN32_THEME_NOT_ACTIVE);
    assert(palettes_equal(&resolved, &normal));

    /* Enabling High Contrast resolves from the stable normal palette. */
    assert(deck_ui_win32_theme_palette_resolve(
               &normal, &colors, 1, &resolved) == DECK_UI_WIN32_THEME_OK);
    assert(palettes_equal(&resolved, &high_contrast));

    /* The resolver also supports true in-place operation. */
    resolved = normal;
    assert(deck_ui_win32_theme_palette_resolve(
               &resolved, &colors, 1, &resolved) == DECK_UI_WIN32_THEME_OK);
    assert(palettes_equal(&resolved, &high_contrast));

    resolved = normal;
    assert(deck_ui_win32_theme_palette_resolve(
               &resolved, NULL, 0, &resolved) == DECK_UI_WIN32_THEME_NOT_ACTIVE);
    assert(palettes_equal(&resolved, &normal));
}

static void test_palette_resolve_rejects_invalid_input_transactionally(void) {
    const DeckUiWin32SystemColors colors = sample_system_colors();
    DeckUiPalette normal;
    DeckUiPalette resolved;
    DeckUiPalette before;

    deck_ui_palette_init(&normal);
    memset(&resolved, 0x5A, sizeof(resolved));
    before = resolved;

    assert(deck_ui_win32_theme_palette_resolve(
               &normal, NULL, 1, &resolved) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&resolved, &before));

    assert(deck_ui_win32_theme_palette_resolve(
               &normal, &colors, 2, &resolved) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&resolved, &before));

    assert(deck_ui_win32_theme_palette_resolve(
               NULL, &colors, 1, &resolved) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&resolved, &before));

    assert(deck_ui_win32_theme_palette_resolve(
               &normal, &colors, 0, NULL) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&resolved, &before));
}

static void test_refresh_palette_uses_current_system_mode(void) {
    DeckUiPalette normal;
    DeckUiPalette resolved;
    DeckUiPalette before;
    DeckUiWin32ThemeStatus status;

    deck_ui_palette_init(&normal);
    memset(&resolved, 0xA5, sizeof(resolved));
    before = resolved;

    status = deck_ui_win32_theme_refresh_palette(&normal, &resolved);
    assert(status == DECK_UI_WIN32_THEME_OK ||
           status == DECK_UI_WIN32_THEME_NOT_ACTIVE ||
           status == DECK_UI_WIN32_THEME_SYSTEM_ERROR);

    if (status == DECK_UI_WIN32_THEME_NOT_ACTIVE) {
        assert(palettes_equal(&resolved, &normal));
    } else if (status == DECK_UI_WIN32_THEME_OK) {
        assert(resolved.colors[DECK_UI_COLOR_CANVAS] ==
               rgb(GetSysColor(COLOR_WINDOW)));
        assert(resolved.colors[DECK_UI_COLOR_TEXT_PRIMARY] ==
               rgb(GetSysColor(COLOR_WINDOWTEXT)));
        assert(resolved.colors[DECK_UI_COLOR_ACCENT] ==
               rgb(GetSysColor(COLOR_HIGHLIGHT)));
    } else {
        assert(palettes_equal(&resolved, &before));
    }

    before = resolved;
    assert(deck_ui_win32_theme_refresh_palette(NULL, &resolved) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
    assert(palettes_equal(&resolved, &before));
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
        assert(palettes_equal(&palette, &before));
    } else {
        assert(palette.colors[DECK_UI_COLOR_TEXT_ON_ACCENT] ==
               rgb(GetSysColor(COLOR_HIGHLIGHTTEXT)));
    }
    assert(deck_ui_win32_theme_query_palette(NULL) ==
           DECK_UI_WIN32_THEME_INVALID_ARGUMENT);
}

int main(void) {
    test_system_color_mapping_is_semantic_and_transactional();
    test_palette_resolve_restores_normal_palette_when_disabled();
    test_palette_resolve_rejects_invalid_input_transactionally();
    test_refresh_palette_uses_current_system_mode();
    test_query_preserves_palette_when_not_active_or_unavailable();
    puts("ui_win32_theme_test: High Contrast transitions and query OK");
    return 0;
}
