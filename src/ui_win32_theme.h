#ifndef CDECK_UI_WIN32_THEME_H
#define CDECK_UI_WIN32_THEME_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DeckUiWin32ThemeStatus {
    DECK_UI_WIN32_THEME_OK = 0,
    DECK_UI_WIN32_THEME_NOT_ACTIVE,
    DECK_UI_WIN32_THEME_INVALID_ARGUMENT,
    DECK_UI_WIN32_THEME_SYSTEM_ERROR
} DeckUiWin32ThemeStatus;

/*
 * Snapshot of the system colors used by the High Contrast palette adapter.
 * Values use the native Win32 COLORREF representation.
 */
typedef struct DeckUiWin32SystemColors {
    COLORREF window;
    COLORREF window_text;
    COLORREF button_face;
    COLORREF button_highlight;
    COLORREF three_d_shadow;
    COLORREF gray_text;
    COLORREF highlight;
    COLORREF highlight_text;
    COLORREF hotlight;
    COLORREF info_text;
    COLORREF info_background;
} DeckUiWin32SystemColors;

/*
 * Maps a supplied system-color snapshot into the semantic C-Deck palette.
 * This deterministic entry point is useful for tests; the output is unchanged
 * when an argument is invalid.
 */
DeckUiWin32ThemeStatus deck_ui_win32_theme_palette_from_system_colors(
    const DeckUiWin32SystemColors *system_colors,
    DeckUiPalette *palette);

/*
 * Queries SPI_GETHIGHCONTRAST. Returns OK and replaces palette only when
 * HCF_HIGHCONTRASTON is enabled. NOT_ACTIVE leaves palette unchanged so the
 * caller can keep the normal C-Deck palette. API failures also preserve it.
 *
 * Call at startup and after WM_SETTINGCHANGE. If the palette changes, destroy
 * and recreate the GDI renderer cache before painting with the new colors.
 */
DeckUiWin32ThemeStatus deck_ui_win32_theme_query_palette(
    DeckUiPalette *palette);

#ifdef __cplusplus
}
#endif

#endif
