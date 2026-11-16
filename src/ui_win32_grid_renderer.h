#ifndef CDECK_UI_WIN32_GRID_RENDERER_H
#define CDECK_UI_WIN32_GRID_RENDERER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui_grid_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DeckUiWin32RenderStatus {
    DECK_UI_WIN32_RENDER_OK = 0,
    DECK_UI_WIN32_RENDER_INVALID_ARGUMENT,
    DECK_UI_WIN32_RENDER_INVALID_FRAME,
    DECK_UI_WIN32_RENDER_GDI_FAILURE
} DeckUiWin32RenderStatus;

typedef struct DeckUiWin32SlotContentLayout {
    RECT icon_bounds;
    RECT label_bounds;
    int has_icon;
    int has_label;
} DeckUiWin32SlotContentLayout;

/*
 * Computes non-overlapping icon/label bounds inside one physical-pixel slot.
 * At compact key sizes the icon is reduced before the label is allowed to
 * overlap it. dpi must be in [48, 768]; has_icon/has_label must be 0 or 1.
 * The output is unchanged when the arguments or slot geometry are invalid.
 */
DeckUiWin32RenderStatus deck_ui_win32_grid_renderer_content_layout(
    const RECT *slot_bounds,
    UINT dpi,
    int has_icon,
    int has_label,
    DeckUiWin32SlotContentLayout *content);

/*
 * A UI-thread-confined GDI resource cache for the native 5x3 deck renderer.
 * The context owns every brush, pen and font stored below. It must be zero
 * initialized before the first init call and destroyed before reuse.
 */
typedef struct DeckUiWin32GridRenderer {
    uint32_t colors[DECK_UI_COLOR_COUNT];
    HBRUSH brushes[DECK_UI_COLOR_COUNT];
    HPEN pens[DECK_UI_COLOR_COUNT];
    HFONT label_font;
    UINT dpi;
    int initialized;
} DeckUiWin32GridRenderer;

/*
 * Creates one cached brush and pen per semantic palette role plus a Segoe UI
 * label font. dpi must be in [48, 768]; zero is not accepted here because a
 * native caller must pass the actual window DPI (or explicitly pass 96).
 *
 * renderer must be zero initialized or previously destroyed. No per-paint
 * GDI objects are created.
 */
DeckUiWin32RenderStatus deck_ui_win32_grid_renderer_init(
    DeckUiWin32GridRenderer *renderer,
    const DeckUiPalette *palette,
    UINT dpi);

/*
 * Draws the 15 validated slot surfaces into an existing HDC. labels and icons
 * are optional arrays of 15 borrowed values; their handles/strings remain
 * owned by the caller. Coordinates are converted from DIP using edge scaling.
 *
 * SaveDC/RestoreDC brackets every paint, so cached GDI objects are never left
 * selected in the target HDC after this function returns.
 */
DeckUiWin32RenderStatus deck_ui_win32_grid_renderer_paint(
    DeckUiWin32GridRenderer *renderer,
    HDC target,
    const DeckUiGridFrame *frame,
    const wchar_t *const labels[DECK_UI_GRID_SLOT_COUNT],
    const HICON icons[DECK_UI_GRID_SLOT_COUNT]);

/*
 * Deletes all owned GDI objects. Returns the number of DeleteObject failures.
 * Handles that could not be deleted remain in the context so a caller may
 * retry after ensuring they are no longer selected in another HDC.
 */
unsigned int deck_ui_win32_grid_renderer_destroy(
    DeckUiWin32GridRenderer *renderer);

#ifdef __cplusplus
}
#endif

#endif
