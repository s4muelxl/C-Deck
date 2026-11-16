#include "ui_win32_grid_renderer.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static int32_t scaled_dip(int32_t value, UINT dpi) {
    int32_t result = deck_ui_dip_to_px(value, (uint32_t)dpi);
    return result > 0 ? result : 1;
}

static int renderer_has_resources(const DeckUiWin32GridRenderer *renderer) {
    size_t index;

    if (!renderer) return 0;
    if (renderer->label_font) return 1;

    for (index = 0u; index < DECK_UI_COLOR_COUNT; ++index) {
        if (renderer->brushes[index] || renderer->pens[index]) return 1;
    }
    return 0;
}

static int find_palette_role(const DeckUiWin32GridRenderer *renderer,
                             uint32_t rgb) {
    size_t index;

    for (index = 0u; index < DECK_UI_COLOR_COUNT; ++index) {
        if (renderer->colors[index] == rgb) return (int)index;
    }
    return -1;
}

static int select_checked(HDC dc, HGDIOBJ object) {
    HGDIOBJ previous;

    if (!dc || !object) return 0;
    previous = SelectObject(dc, object);
    return previous != NULL && previous != HGDI_ERROR;
}

static int rect_to_win32(const DeckUiRect *source, UINT dpi, RECT *target) {
    DeckUiRect scaled;
    int64_t right;
    int64_t bottom;

    if (!source || !target || source->width <= 0 || source->height <= 0) {
        return 0;
    }
    if (deck_ui_rect_dip_to_px(source, (uint32_t)dpi, &scaled) !=
        DECK_UI_RECT_SCALE_OK) {
        return 0;
    }

    right = (int64_t)scaled.x + scaled.width;
    bottom = (int64_t)scaled.y + scaled.height;
    if (right > LONG_MAX || right < LONG_MIN ||
        bottom > LONG_MAX || bottom < LONG_MIN) {
        return 0;
    }

    target->left = (LONG)scaled.x;
    target->top = (LONG)scaled.y;
    target->right = (LONG)right;
    target->bottom = (LONG)bottom;
    return target->right > target->left && target->bottom > target->top;
}

static int grid_geometry_is_valid(const DeckUiGridFrame *frame) {
    const DeckUiRect *first;
    int64_t gap_x;
    int64_t gap_y;
    uint32_t index;

    if (!frame) return 0;
    first = &frame->slots[0].bounds;
    if (first->width <= 0 || first->height != first->width) return 0;

    gap_x = (int64_t)frame->slots[1].bounds.x -
            ((int64_t)first->x + first->width);
    gap_y = (int64_t)frame->slots[DECK_UI_GRID_COLUMNS].bounds.y -
            ((int64_t)first->y + first->height);
    if (gap_x <= 0 || gap_y <= 0 || gap_x != gap_y) return 0;

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiRect *slot = &frame->slots[index].bounds;
        const int64_t row = index / DECK_UI_GRID_COLUMNS;
        const int64_t column = index % DECK_UI_GRID_COLUMNS;
        const int64_t expected_x = (int64_t)first->x +
            column * ((int64_t)first->width + gap_x);
        const int64_t expected_y = (int64_t)first->y +
            row * ((int64_t)first->height + gap_y);

        if (slot->width != first->width || slot->height != first->height ||
            (int64_t)slot->x != expected_x ||
            (int64_t)slot->y != expected_y) {
            return 0;
        }
    }

    return 1;
}

static int frame_is_valid(const DeckUiWin32GridRenderer *renderer,
                          const DeckUiGridFrame *frame) {
    DeckUiPalette palette;
    uint32_t index;

    if (!renderer || !renderer->initialized || !frame ||
        frame->slot_count != DECK_UI_GRID_SLOT_COUNT ||
        frame->layout_mode <= DECK_UI_LAYOUT_MODE_UNSUPPORTED ||
        frame->layout_mode > DECK_UI_LAYOUT_MODE_COMPACT ||
        !grid_geometry_is_valid(frame)) {
        return 0;
    }

    memcpy(palette.colors, renderer->colors, sizeof(palette.colors));
    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiGridSlotFrame *slot = &frame->slots[index];
        DeckUiStyle expected_style;
        RECT bounds;

        if (slot->presentation.visual_state < DECK_UI_STATE_NORMAL ||
            slot->presentation.visual_state > DECK_UI_STATE_WARNING ||
            (slot->presentation.focused != 0 &&
             slot->presentation.focused != 1) ||
            (slot->style.flags & ~DECK_UI_STYLE_FOCUS_RING) != 0u ||
            find_palette_role(renderer, slot->style.fill) < 0 ||
            find_palette_role(renderer, slot->style.text) < 0 ||
            find_palette_role(renderer, slot->style.border) < 0 ||
            find_palette_role(renderer, slot->style.indicator) < 0 ||
            !rect_to_win32(&slot->bounds, renderer->dpi, &bounds)) {
            return 0;
        }

        expected_style = deck_ui_style_for_interaction(
            &palette, slot->presentation.visual_state,
            slot->presentation.focused);
        if (slot->style.fill != expected_style.fill ||
            slot->style.text != expected_style.text ||
            slot->style.border != expected_style.border ||
            slot->style.indicator != expected_style.indicator ||
            slot->style.flags != expected_style.flags) {
            return 0;
        }
    }

    return 1;
}

DeckUiWin32RenderStatus deck_ui_win32_grid_renderer_init(
    DeckUiWin32GridRenderer *renderer,
    const DeckUiPalette *palette,
    UINT dpi) {
    DeckUiWin32GridRenderer candidate;
    size_t index;
    int32_t pen_width;
    int32_t focus_width;
    int32_t font_height;

    if (!renderer || !palette || dpi < 48u || dpi > 768u ||
        renderer->initialized || renderer_has_resources(renderer)) {
        return DECK_UI_WIN32_RENDER_INVALID_ARGUMENT;
    }

    for (index = 0u; index < DECK_UI_COLOR_COUNT; ++index) {
        if (palette->colors[index] > 0xFFFFFFu) {
            return DECK_UI_WIN32_RENDER_INVALID_ARGUMENT;
        }
    }

    memset(&candidate, 0, sizeof(candidate));
    candidate.dpi = dpi;
    pen_width = scaled_dip(1, dpi);
    focus_width = scaled_dip(2, dpi);
    font_height = scaled_dip(12, dpi);

    for (index = 0u; index < DECK_UI_COLOR_COUNT; ++index) {
        COLORREF color = (COLORREF)deck_ui_rgb_to_colorref(
            palette->colors[index]);
        int width = index == DECK_UI_COLOR_FOCUS
            ? (focus_width > 1 ? focus_width : 2)
            : pen_width;

        candidate.colors[index] = palette->colors[index];
        candidate.brushes[index] = CreateSolidBrush(color);
        candidate.pens[index] = CreatePen(PS_SOLID, width, color);
        if (!candidate.brushes[index] || !candidate.pens[index]) {
            (void)deck_ui_win32_grid_renderer_destroy(&candidate);
            return DECK_UI_WIN32_RENDER_GDI_FAILURE;
        }
    }

    candidate.label_font = CreateFontW(
        -font_height, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    if (!candidate.label_font) {
        (void)deck_ui_win32_grid_renderer_destroy(&candidate);
        return DECK_UI_WIN32_RENDER_GDI_FAILURE;
    }

    candidate.initialized = 1;
    *renderer = candidate;
    return DECK_UI_WIN32_RENDER_OK;
}

DeckUiWin32RenderStatus deck_ui_win32_grid_renderer_paint(
    DeckUiWin32GridRenderer *renderer,
    HDC target,
    const DeckUiGridFrame *frame,
    const wchar_t *const labels[DECK_UI_GRID_SLOT_COUNT],
    const HICON icons[DECK_UI_GRID_SLOT_COUNT]) {
    int saved_state;
    int failed = 0;
    uint32_t index;
    HGDIOBJ null_brush;

    if (!renderer || !target || !frame || !renderer->initialized) {
        return DECK_UI_WIN32_RENDER_INVALID_ARGUMENT;
    }
    if (!frame_is_valid(renderer, frame)) {
        return DECK_UI_WIN32_RENDER_INVALID_FRAME;
    }

    saved_state = SaveDC(target);
    if (saved_state == 0) return DECK_UI_WIN32_RENDER_GDI_FAILURE;

    null_brush = GetStockObject(NULL_BRUSH);
    if (!null_brush || SetBkMode(target, TRANSPARENT) == 0 ||
        !select_checked(target, renderer->label_font)) {
        failed = 1;
    }

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT && !failed; ++index) {
        const DeckUiGridSlotFrame *slot = &frame->slots[index];
        RECT bounds;
        int fill_role = find_palette_role(renderer, slot->style.fill);
        int border_role = find_palette_role(renderer, slot->style.border);
        int indicator_role = find_palette_role(renderer, slot->style.indicator);
        int32_t radius;
        int32_t pad;
        int32_t indicator_width;
        int32_t indicator_half_height;

        if (!rect_to_win32(&slot->bounds, renderer->dpi, &bounds)) {
            failed = 1;
            break;
        }

        radius = scaled_dip(8, renderer->dpi) * 2;
        pad = scaled_dip(6, renderer->dpi);
        indicator_width = scaled_dip(3, renderer->dpi);
        indicator_half_height = scaled_dip(9, renderer->dpi);

        if (!select_checked(target, renderer->brushes[fill_role]) ||
            !select_checked(target, renderer->pens[border_role]) ||
            !RoundRect(target, bounds.left, bounds.top, bounds.right,
                       bounds.bottom, radius, radius)) {
            failed = 1;
            break;
        }

        if (indicator_role >= 0 && indicator_role != fill_role) {
            int32_t left = bounds.left + scaled_dip(7, renderer->dpi);
            int32_t center_y = bounds.top +
                (bounds.bottom - bounds.top) / 2;
            RECT indicator;

            indicator.left = left;
            indicator.right = left + indicator_width;
            indicator.top = center_y - indicator_half_height;
            indicator.bottom = center_y + indicator_half_height;
            if (!select_checked(target, renderer->brushes[indicator_role]) ||
                !select_checked(target, renderer->pens[indicator_role]) ||
                !RoundRect(target, indicator.left, indicator.top,
                           indicator.right, indicator.bottom,
                           indicator_width * 2, indicator_width * 2)) {
                failed = 1;
                break;
            }
        }

        if (icons && icons[index]) {
            int32_t icon_size = scaled_dip(24, renderer->dpi);
            int32_t icon_x = bounds.left +
                ((bounds.right - bounds.left) - icon_size) / 2;
            int32_t icon_y = bounds.top + scaled_dip(8, renderer->dpi);

            if (!DrawIconEx(target, icon_x, icon_y, icons[index],
                            icon_size, icon_size, 0u, NULL, DI_NORMAL)) {
                failed = 1;
                break;
            }
        }

        if (labels && labels[index] && labels[index][0] != L'\0') {
            RECT label_bounds;
            int32_t label_height = scaled_dip(18, renderer->dpi);
            int32_t bottom_pad = scaled_dip(5, renderer->dpi);

            label_bounds.left = bounds.left + pad;
            label_bounds.right = bounds.right - pad;
            label_bounds.bottom = bounds.bottom - bottom_pad;
            label_bounds.top = label_bounds.bottom - label_height;

            if (label_bounds.right <= label_bounds.left ||
                label_bounds.bottom <= label_bounds.top ||
                SetTextColor(target, (COLORREF)deck_ui_rgb_to_colorref(
                    slot->style.text)) == CLR_INVALID ||
                DrawTextW(target, labels[index], -1, &label_bounds,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE |
                          DT_END_ELLIPSIS | DT_NOPREFIX) == 0) {
                failed = 1;
                break;
            }
        }

        if ((slot->style.flags & DECK_UI_STYLE_FOCUS_RING) != 0u) {
            int32_t inset = scaled_dip(4, renderer->dpi);
            RECT focus_bounds;

            focus_bounds.left = bounds.left + inset;
            focus_bounds.top = bounds.top + inset;
            focus_bounds.right = bounds.right - inset;
            focus_bounds.bottom = bounds.bottom - inset;

            if (focus_bounds.right <= focus_bounds.left ||
                focus_bounds.bottom <= focus_bounds.top ||
                !select_checked(target, null_brush) ||
                !select_checked(target, renderer->pens[DECK_UI_COLOR_FOCUS]) ||
                !RoundRect(target, focus_bounds.left, focus_bounds.top,
                           focus_bounds.right, focus_bounds.bottom,
                           scaled_dip(6, renderer->dpi) * 2,
                           scaled_dip(6, renderer->dpi) * 2)) {
                failed = 1;
                break;
            }
        }
    }

    if (!RestoreDC(target, saved_state)) {
        return DECK_UI_WIN32_RENDER_GDI_FAILURE;
    }
    return failed ? DECK_UI_WIN32_RENDER_GDI_FAILURE : DECK_UI_WIN32_RENDER_OK;
}

unsigned int deck_ui_win32_grid_renderer_destroy(
    DeckUiWin32GridRenderer *renderer) {
    unsigned int failures = 0u;
    size_t index;

    if (!renderer) return 1u;

    for (index = 0u; index < DECK_UI_COLOR_COUNT; ++index) {
        if (renderer->pens[index]) {
            if (DeleteObject(renderer->pens[index])) {
                renderer->pens[index] = NULL;
            } else {
                ++failures;
            }
        }
        if (renderer->brushes[index]) {
            if (DeleteObject(renderer->brushes[index])) {
                renderer->brushes[index] = NULL;
            } else {
                ++failures;
            }
        }
    }

    if (renderer->label_font) {
        if (DeleteObject(renderer->label_font)) {
            renderer->label_font = NULL;
        } else {
            ++failures;
        }
    }

    renderer->initialized = 0;
    if (!renderer_has_resources(renderer)) {
        memset(renderer, 0, sizeof(*renderer));
    }
    return failures;
}
