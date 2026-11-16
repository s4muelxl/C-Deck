#ifndef CDECK_UI_THEME_H
#define CDECK_UI_THEME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DeckUiColorRole {
    DECK_UI_COLOR_CANVAS = 0,
    DECK_UI_COLOR_SURFACE_1,
    DECK_UI_COLOR_SURFACE_2,
    DECK_UI_COLOR_SURFACE_3,
    DECK_UI_COLOR_BORDER_SUBTLE,
    DECK_UI_COLOR_BORDER_STRONG,
    DECK_UI_COLOR_TEXT_PRIMARY,
    DECK_UI_COLOR_TEXT_SECONDARY,
    DECK_UI_COLOR_TEXT_DISABLED,
    DECK_UI_COLOR_ACCENT,
    DECK_UI_COLOR_ACCENT_BACKGROUND,
    DECK_UI_COLOR_DANGER,
    DECK_UI_COLOR_WARNING,
    DECK_UI_COLOR_FOCUS,
    DECK_UI_COLOR_COUNT
} DeckUiColorRole;

typedef struct DeckUiPalette {
    /* Colors use 0xRRGGBB, independent of Win32 COLORREF byte ordering. */
    uint32_t colors[DECK_UI_COLOR_COUNT];
} DeckUiPalette;

typedef enum DeckUiVisualState {
    DECK_UI_STATE_NORMAL = 0,
    DECK_UI_STATE_HOVER,
    DECK_UI_STATE_PRESSED,
    DECK_UI_STATE_SELECTED,
    DECK_UI_STATE_DISABLED,
    DECK_UI_STATE_FOCUSED,
    DECK_UI_STATE_ERROR,
    DECK_UI_STATE_WARNING
} DeckUiVisualState;

enum {
    DECK_UI_STYLE_NONE = 0u,
    DECK_UI_STYLE_FOCUS_RING = 1u << 0
};

typedef struct DeckUiStyle {
    uint32_t fill;
    uint32_t text;
    uint32_t border;
    uint32_t indicator;
    unsigned int flags;
} DeckUiStyle;

/* Initializes the stable C-Deck dark palette. No allocation or Win32 dependency. */
void deck_ui_palette_init(DeckUiPalette *palette);

/* Returns 0 for a null palette or an invalid role. */
uint32_t deck_ui_palette_get(const DeckUiPalette *palette, DeckUiColorRole role);

/* Converts palette RGB (0xRRGGBB) to Win32 COLORREF (0x00BBGGRR).
 * Kept free of windows.h so the semantic theme remains portable C11.
 */
uint32_t deck_ui_rgb_to_colorref(uint32_t rgb);

/* Resolves semantic component states. Invalid states safely fall back to NORMAL. */
DeckUiStyle deck_ui_style_for_state(const DeckUiPalette *palette, DeckUiVisualState state);

#ifdef __cplusplus
}
#endif

#endif
