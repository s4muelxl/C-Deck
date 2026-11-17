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
 * Resolves a complete palette from a stable normal palette and an explicit
 * High Contrast state. When disabled, the normal palette is copied to the
 * output (including when it previously held High Contrast colors). When
 * enabled, system_colors is required and supplies the system palette.
 *
 * The operation is transactional: invalid arguments leave the output intact.
 * The output may alias normal_palette.
 */
DeckUiWin32ThemeStatus deck_ui_win32_theme_palette_resolve(
    const DeckUiPalette *normal_palette,
    const DeckUiWin32SystemColors *system_colors,
    int high_contrast_enabled,
    DeckUiPalette *resolved_palette);

/*
 * Low-level query of SPI_GETHIGHCONTRAST. Returns OK and replaces palette only
 * when HCF_HIGHCONTRASTON is enabled. NOT_ACTIVE leaves palette unchanged.
 * Prefer refresh_palette for WM_SETTINGCHANGE handling because it restores
 * the supplied normal palette when High Contrast has been turned off.
 */
DeckUiWin32ThemeStatus deck_ui_win32_theme_query_palette(
    DeckUiPalette *palette);

/*
 * Resolves the current system mode against the stable normal palette.
 * Returns OK with system colors when High Contrast is active; returns
 * NOT_ACTIVE and copies normal_palette when it is inactive. System API
 * failures preserve resolved_palette. normal_palette and resolved_palette
 * may alias.
 *
 * Call at startup and after WM_SETTINGCHANGE. If the resolved palette differs
 * from the active renderer palette, build a replacement GDI cache before
 * switching the UI-thread-owned renderer. This function does not mutate
 * system settings or create GDI objects.
 */
DeckUiWin32ThemeStatus deck_ui_win32_theme_refresh_palette(
    const DeckUiPalette *normal_palette,
    DeckUiPalette *resolved_palette);

#ifdef __cplusplus
}
#endif

#endif
