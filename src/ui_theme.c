#include "ui_theme.h"

#include <stddef.h>
#include <string.h>

void deck_ui_palette_init(DeckUiPalette *palette) {
    static const uint32_t defaults[DECK_UI_COLOR_COUNT] = {
        0x0B0D10u, /* canvas */
        0x12161Bu, /* surface 1 */
        0x191F26u, /* surface 2 */
        0x222A33u, /* surface 3 */
        0x2B333Du, /* subtle border */
        0x46515Eu, /* strong border */
        0xF3F5F7u, /* primary text */
        0xA5AFBAu, /* secondary text */
        0x69737Fu, /* disabled text */
        0x9DFF00u, /* official C-Deck lime accent */
        0x18281Au, /* selected surface */
        0xF06B6Bu, /* danger */
        0xE6B85Cu, /* warning */
        0xFFFFFFu  /* keyboard focus */
    };

    if (!palette) return;
    memcpy(palette->colors, defaults, sizeof(defaults));
}

uint32_t deck_ui_palette_get(const DeckUiPalette *palette, DeckUiColorRole role) {
    if (!palette || role < 0 || role >= DECK_UI_COLOR_COUNT) return 0u;
    return palette->colors[role];
}

static DeckUiStyle make_style(const DeckUiPalette *palette,
                              DeckUiColorRole fill,
                              DeckUiColorRole text,
                              DeckUiColorRole border,
                              DeckUiColorRole indicator,
                              unsigned int flags) {
    DeckUiStyle style;
    style.fill = deck_ui_palette_get(palette, fill);
    style.text = deck_ui_palette_get(palette, text);
    style.border = deck_ui_palette_get(palette, border);
    style.indicator = deck_ui_palette_get(palette, indicator);
    style.flags = flags;
    return style;
}

DeckUiStyle deck_ui_style_for_state(const DeckUiPalette *palette, DeckUiVisualState state) {
    DeckUiStyle empty = { 0u, 0u, 0u, 0u, DECK_UI_STYLE_NONE };

    if (!palette) return empty;

    switch (state) {
        case DECK_UI_STATE_HOVER:
            return make_style(palette, DECK_UI_COLOR_SURFACE_3,
                              DECK_UI_COLOR_TEXT_PRIMARY, DECK_UI_COLOR_BORDER_STRONG,
                              DECK_UI_COLOR_SURFACE_3, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_PRESSED:
            return make_style(palette, DECK_UI_COLOR_CANVAS,
                              DECK_UI_COLOR_TEXT_PRIMARY, DECK_UI_COLOR_BORDER_STRONG,
                              DECK_UI_COLOR_BORDER_STRONG, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_SELECTED:
            return make_style(palette, DECK_UI_COLOR_ACCENT_BACKGROUND,
                              DECK_UI_COLOR_TEXT_PRIMARY, DECK_UI_COLOR_ACCENT,
                              DECK_UI_COLOR_ACCENT, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_DISABLED:
            return make_style(palette, DECK_UI_COLOR_SURFACE_1,
                              DECK_UI_COLOR_TEXT_DISABLED, DECK_UI_COLOR_BORDER_SUBTLE,
                              DECK_UI_COLOR_TEXT_DISABLED, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_FOCUSED:
            return make_style(palette, DECK_UI_COLOR_SURFACE_2,
                              DECK_UI_COLOR_TEXT_PRIMARY, DECK_UI_COLOR_FOCUS,
                              DECK_UI_COLOR_FOCUS, DECK_UI_STYLE_FOCUS_RING);
        case DECK_UI_STATE_ERROR:
            return make_style(palette, DECK_UI_COLOR_SURFACE_1,
                              DECK_UI_COLOR_DANGER, DECK_UI_COLOR_DANGER,
                              DECK_UI_COLOR_DANGER, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_WARNING:
            return make_style(palette, DECK_UI_COLOR_SURFACE_1,
                              DECK_UI_COLOR_WARNING, DECK_UI_COLOR_WARNING,
                              DECK_UI_COLOR_WARNING, DECK_UI_STYLE_NONE);
        case DECK_UI_STATE_NORMAL:
        default:
            return make_style(palette, DECK_UI_COLOR_SURFACE_2,
                              DECK_UI_COLOR_TEXT_PRIMARY, DECK_UI_COLOR_BORDER_SUBTLE,
                              DECK_UI_COLOR_SURFACE_2, DECK_UI_STYLE_NONE);
    }
}
