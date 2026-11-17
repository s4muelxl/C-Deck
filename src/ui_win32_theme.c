#include "ui_win32_theme.h"

#include <string.h>

static uint32_t colorref_to_rgb(COLORREF color) {
    return ((uint32_t)GetRValue(color) << 16u) |
           ((uint32_t)GetGValue(color) << 8u) |
           (uint32_t)GetBValue(color);
}

static void query_system_colors(DeckUiWin32SystemColors *system_colors) {
    system_colors->window = GetSysColor(COLOR_WINDOW);
    system_colors->window_text = GetSysColor(COLOR_WINDOWTEXT);
    system_colors->button_face = GetSysColor(COLOR_BTNFACE);
    system_colors->button_highlight = GetSysColor(COLOR_BTNHIGHLIGHT);
    system_colors->three_d_shadow = GetSysColor(COLOR_3DSHADOW);
    system_colors->gray_text = GetSysColor(COLOR_GRAYTEXT);
    system_colors->highlight = GetSysColor(COLOR_HIGHLIGHT);
    system_colors->highlight_text = GetSysColor(COLOR_HIGHLIGHTTEXT);
    system_colors->hotlight = GetSysColor(COLOR_HOTLIGHT);
    system_colors->info_text = GetSysColor(COLOR_INFOTEXT);
    system_colors->info_background = GetSysColor(COLOR_INFOBK);
}

DeckUiWin32ThemeStatus deck_ui_win32_theme_palette_from_system_colors(
    const DeckUiWin32SystemColors *system_colors,
    DeckUiPalette *palette) {
    DeckUiPalette candidate;

    if (!system_colors || !palette) {
        return DECK_UI_WIN32_THEME_INVALID_ARGUMENT;
    }

    deck_ui_palette_init(&candidate);
    candidate.colors[DECK_UI_COLOR_CANVAS] =
        colorref_to_rgb(system_colors->window);
    candidate.colors[DECK_UI_COLOR_SURFACE_1] =
        colorref_to_rgb(system_colors->button_face);
    candidate.colors[DECK_UI_COLOR_SURFACE_2] =
        colorref_to_rgb(system_colors->window);
    candidate.colors[DECK_UI_COLOR_SURFACE_3] =
        colorref_to_rgb(system_colors->button_highlight);
    candidate.colors[DECK_UI_COLOR_BORDER_SUBTLE] =
        colorref_to_rgb(system_colors->three_d_shadow);
    candidate.colors[DECK_UI_COLOR_BORDER_STRONG] =
        colorref_to_rgb(system_colors->window_text);
    candidate.colors[DECK_UI_COLOR_TEXT_PRIMARY] =
        colorref_to_rgb(system_colors->window_text);
    candidate.colors[DECK_UI_COLOR_TEXT_SECONDARY] =
        colorref_to_rgb(system_colors->window_text);
    candidate.colors[DECK_UI_COLOR_TEXT_DISABLED] =
        colorref_to_rgb(system_colors->gray_text);
    candidate.colors[DECK_UI_COLOR_TEXT_ON_ACCENT] =
        colorref_to_rgb(system_colors->highlight_text);
    candidate.colors[DECK_UI_COLOR_ACCENT] =
        colorref_to_rgb(system_colors->highlight);
    candidate.colors[DECK_UI_COLOR_ACCENT_BACKGROUND] =
        colorref_to_rgb(system_colors->highlight);
    candidate.colors[DECK_UI_COLOR_DANGER] =
        colorref_to_rgb(system_colors->hotlight);
    candidate.colors[DECK_UI_COLOR_DANGER_BACKGROUND] =
        colorref_to_rgb(system_colors->window);
    candidate.colors[DECK_UI_COLOR_WARNING] =
        colorref_to_rgb(system_colors->info_text);
    candidate.colors[DECK_UI_COLOR_WARNING_BACKGROUND] =
        colorref_to_rgb(system_colors->info_background);
    candidate.colors[DECK_UI_COLOR_FOCUS] =
        colorref_to_rgb(system_colors->highlight);

    *palette = candidate;
    return DECK_UI_WIN32_THEME_OK;
}

DeckUiWin32ThemeStatus deck_ui_win32_theme_palette_resolve(
    const DeckUiPalette *normal_palette,
    const DeckUiWin32SystemColors *system_colors,
    int high_contrast_enabled,
    DeckUiPalette *resolved_palette) {
    DeckUiPalette candidate;

    if (!normal_palette || !resolved_palette ||
        (high_contrast_enabled != 0 && high_contrast_enabled != 1) ||
        (high_contrast_enabled && !system_colors)) {
        return DECK_UI_WIN32_THEME_INVALID_ARGUMENT;
    }

    candidate = *normal_palette;
    if (high_contrast_enabled) {
        DeckUiWin32ThemeStatus status =
            deck_ui_win32_theme_palette_from_system_colors(
                system_colors, &candidate);
        if (status != DECK_UI_WIN32_THEME_OK) return status;
    }

    *resolved_palette = candidate;
    return high_contrast_enabled
        ? DECK_UI_WIN32_THEME_OK
        : DECK_UI_WIN32_THEME_NOT_ACTIVE;
}

DeckUiWin32ThemeStatus deck_ui_win32_theme_query_palette(
    DeckUiPalette *palette) {
    HIGHCONTRASTW high_contrast;
    DeckUiWin32SystemColors system_colors;

    if (!palette) return DECK_UI_WIN32_THEME_INVALID_ARGUMENT;

    memset(&high_contrast, 0, sizeof(high_contrast));
    high_contrast.cbSize = (UINT)sizeof(high_contrast);
    if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST,
                               (UINT)sizeof(high_contrast),
                               &high_contrast, 0u)) {
        return DECK_UI_WIN32_THEME_SYSTEM_ERROR;
    }

    if ((high_contrast.dwFlags & HCF_HIGHCONTRASTON) == 0u) {
        return DECK_UI_WIN32_THEME_NOT_ACTIVE;
    }

    query_system_colors(&system_colors);
    return deck_ui_win32_theme_palette_from_system_colors(
        &system_colors, palette);
}

DeckUiWin32ThemeStatus deck_ui_win32_theme_refresh_palette(
    const DeckUiPalette *normal_palette,
    DeckUiPalette *resolved_palette) {
    HIGHCONTRASTW high_contrast;
    DeckUiWin32SystemColors system_colors;
    DeckUiPalette candidate;

    if (!normal_palette || !resolved_palette) {
        return DECK_UI_WIN32_THEME_INVALID_ARGUMENT;
    }

    memset(&high_contrast, 0, sizeof(high_contrast));
    high_contrast.cbSize = (UINT)sizeof(high_contrast);
    if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST,
                               (UINT)sizeof(high_contrast),
                               &high_contrast, 0u)) {
        return DECK_UI_WIN32_THEME_SYSTEM_ERROR;
    }

    if ((high_contrast.dwFlags & HCF_HIGHCONTRASTON) == 0u) {
        *resolved_palette = *normal_palette;
        return DECK_UI_WIN32_THEME_NOT_ACTIVE;
    }

    query_system_colors(&system_colors);
    candidate = *normal_palette;
    if (deck_ui_win32_theme_palette_from_system_colors(
            &system_colors, &candidate) != DECK_UI_WIN32_THEME_OK) {
        return DECK_UI_WIN32_THEME_SYSTEM_ERROR;
    }

    *resolved_palette = candidate;
    return DECK_UI_WIN32_THEME_OK;
}
