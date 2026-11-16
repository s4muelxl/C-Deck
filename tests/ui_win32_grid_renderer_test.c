#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui_win32_grid_renderer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static RECT scale_rect(const DeckUiRect *source, UINT dpi) {
    DeckUiRect scaled;
    RECT result;

    assert(deck_ui_rect_dip_to_px(source, (uint32_t)dpi, &scaled) ==
           DECK_UI_RECT_SCALE_OK);
    result.left = scaled.x;
    result.top = scaled.y;
    result.right = scaled.x + scaled.width;
    result.bottom = scaled.y + scaled.height;
    return result;
}

static COLORREF pixel_at(HDC dc, const RECT *rect, int32_t x_offset,
                         int32_t y_offset) {
    COLORREF color = GetPixel(dc, rect->left + x_offset, rect->top + y_offset);
    assert(color != CLR_INVALID);
    return color;
}

static void test_content_layout_avoids_icon_label_overlap(void) {
    DeckUiWin32SlotContentLayout content;
    DeckUiWin32SlotContentLayout before;
    RECT compact_96 = { 0, 0, 44, 44 };
    RECT compact_48 = { 0, 0, 22, 22 };
    RECT icon_only = { 0, 0, 44, 44 };

    assert(deck_ui_win32_grid_renderer_content_layout(
               &compact_96, 96u, 1, 1, &content) ==
           DECK_UI_WIN32_RENDER_OK);
    assert(content.has_icon == 1 && content.has_label == 1);
    assert(content.icon_bounds.right - content.icon_bounds.left ==
           content.icon_bounds.bottom - content.icon_bounds.top);
    assert(content.icon_bounds.bottom <= content.label_bounds.top - 3);
    assert(content.icon_bounds.left >= compact_96.left);
    assert(content.icon_bounds.right <= compact_96.right);

    assert(deck_ui_win32_grid_renderer_content_layout(
               &compact_48, 48u, 1, 1, &content) ==
           DECK_UI_WIN32_RENDER_OK);
    assert(content.has_icon == 1 && content.has_label == 1);
    assert(content.icon_bounds.bottom <= content.label_bounds.top - 2);

    assert(deck_ui_win32_grid_renderer_content_layout(
               &icon_only, 96u, 1, 0, &content) ==
           DECK_UI_WIN32_RENDER_OK);
    assert(content.has_icon == 1 && content.has_label == 0);
    assert(content.icon_bounds.top > icon_only.top);
    assert(content.icon_bounds.bottom < icon_only.bottom);

    assert(deck_ui_win32_grid_renderer_content_layout(
               &icon_only, 96u, 0, 1, &content) ==
           DECK_UI_WIN32_RENDER_OK);
    assert(content.has_icon == 0 && content.has_label == 1);

    before = content;
    assert(deck_ui_win32_grid_renderer_content_layout(
               &icon_only, 47u, 1, 1, &content) ==
           DECK_UI_WIN32_RENDER_INVALID_ARGUMENT);
    assert(memcmp(&content, &before, sizeof(content)) == 0);
    assert(deck_ui_win32_grid_renderer_content_layout(
               &icon_only, 96u, 2, 1, &content) ==
           DECK_UI_WIN32_RENDER_INVALID_ARGUMENT);
    assert(memcmp(&content, &before, sizeof(content)) == 0);
}

int main(void) {
    test_content_layout_avoids_icon_label_overlap();

    HDC screen_dc = GetDC(NULL);
    HDC memory_dc;
    HBITMAP bitmap;
    HGDIOBJ previous_bitmap;
    HGDIOBJ previous_brush;
    HGDIOBJ previous_pen;
    HGDIOBJ previous_font;
    DeckUiPalette palette;
    DeckUiLayout layout;
    DeckUiLayoutInput input = { 900, 700, 0 };
    DeckUiViewState view;
    DeckUiVisualState model_states[DECK_UI_GRID_SLOT_COUNT] = {
        DECK_UI_STATE_NORMAL
    };
    DeckUiGridFrame frame;
    DeckUiWin32GridRenderer renderer;
    const wchar_t *labels[DECK_UI_GRID_SLOT_COUNT] = { NULL };
    HICON icons[DECK_UI_GRID_SLOT_COUNT] = { NULL };
    RECT slot_zero;
    RECT slot_seven;
    RECT slot_eight;
    DWORD resources_after_init;
    DWORD resources_after_warmup;
    DWORD resources_after_paint;
    DWORD resources_before_init;
    DWORD resources_after_destroy;
    unsigned int iteration;
    const UINT dpi = 144u;

    assert(screen_dc != NULL);
    memory_dc = CreateCompatibleDC(screen_dc);
    assert(memory_dc != NULL);
    bitmap = CreateCompatibleBitmap(screen_dc, 2000, 1500);
    assert(bitmap != NULL);
    previous_bitmap = SelectObject(memory_dc, bitmap);
    assert(previous_bitmap != NULL && previous_bitmap != HGDI_ERROR);
    assert(PatBlt(memory_dc, 0, 0, 2000, 1500, BLACKNESS));

    deck_ui_palette_init(&palette);
    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    deck_ui_view_state_init(&view);
    assert(deck_ui_view_select(&view, 7u) == DECK_UI_VIEW_SELECTED);
    assert(deck_ui_view_focus(&view, 0u) == DECK_UI_VIEW_OK);
    model_states[8] = DECK_UI_STATE_DISABLED;
    model_states[9] = DECK_UI_STATE_ERROR;
    model_states[10] = DECK_UI_STATE_WARNING;
    labels[14] = L"Áudio";
    icons[13] = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
    assert(icons[13] != NULL);

    assert(deck_ui_grid_frame_build(&layout, &view, &palette,
                                    model_states, &frame) ==
           DECK_UI_GRID_FRAME_OK);

    memset(&renderer, 0, sizeof(renderer));
    resources_before_init = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(deck_ui_win32_grid_renderer_init(&renderer, &palette, dpi) ==
           DECK_UI_WIN32_RENDER_OK);
    resources_after_init = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(resources_after_init > resources_before_init);

    previous_brush = GetCurrentObject(memory_dc, OBJ_BRUSH);
    previous_pen = GetCurrentObject(memory_dc, OBJ_PEN);
    previous_font = GetCurrentObject(memory_dc, OBJ_FONT);
    assert(previous_brush != NULL && previous_pen != NULL &&
           previous_font != NULL);

    /* The first GDI draw may initialize process-wide font/icon caches. */
    assert(deck_ui_win32_grid_renderer_paint(&renderer, memory_dc,
                                              &frame, labels, icons) ==
           DECK_UI_WIN32_RENDER_OK);
    resources_after_warmup = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);

    for (iteration = 0u; iteration < 64u; ++iteration) {
        assert(deck_ui_win32_grid_renderer_paint(&renderer, memory_dc,
                                                  &frame, labels, icons) ==
               DECK_UI_WIN32_RENDER_OK);
    }

    /* A minimum-size compact layout must keep its icon clear of its label. */
    {
        DeckUiLayout compact_layout;
        DeckUiLayoutInput compact_input = { 320, 270, 1 };
        DeckUiViewState compact_view;
        DeckUiGridFrame compact_frame;
        const wchar_t *compact_labels[DECK_UI_GRID_SLOT_COUNT] = { NULL };
        HICON compact_icons[DECK_UI_GRID_SLOT_COUNT] = { NULL };

        assert(deck_ui_layout_compute(&compact_input, &compact_layout) ==
               DECK_UI_LAYOUT_OK);
        assert(compact_layout.slot_size_dip == 45);
        deck_ui_view_state_init(&compact_view);
        compact_labels[0] = L"Compact";
        compact_icons[0] = icons[13];
        assert(deck_ui_grid_frame_build(&compact_layout, &compact_view,
                                        &palette, NULL, &compact_frame) ==
               DECK_UI_GRID_FRAME_OK);
        assert(deck_ui_win32_grid_renderer_paint(
                   &renderer, memory_dc, &compact_frame,
                   compact_labels, compact_icons) ==
               DECK_UI_WIN32_RENDER_OK);
    }

    resources_after_paint = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(resources_after_paint == resources_after_warmup);
    assert(resources_after_init > resources_before_init);
    assert(GetCurrentObject(memory_dc, OBJ_BRUSH) == previous_brush);
    assert(GetCurrentObject(memory_dc, OBJ_PEN) == previous_pen);
    assert(GetCurrentObject(memory_dc, OBJ_FONT) == previous_font);

    slot_zero = scale_rect(&layout.slots[0], dpi);
    slot_seven = scale_rect(&layout.slots[7], dpi);
    slot_eight = scale_rect(&layout.slots[8], dpi);

    /* These centers are unobstructed by labels/icons and expose fill colors. */
    assert(pixel_at(memory_dc, &slot_zero,
                    (slot_zero.right - slot_zero.left) / 2,
                    (slot_zero.bottom - slot_zero.top) / 2) ==
           (COLORREF)deck_ui_rgb_to_colorref(0x191F26u));
    assert(pixel_at(memory_dc, &slot_seven,
                    (slot_seven.right - slot_seven.left) / 2,
                    (slot_seven.bottom - slot_seven.top) / 2) ==
           (COLORREF)deck_ui_rgb_to_colorref(0x18281Au));
    assert(pixel_at(memory_dc, &slot_eight,
                    (slot_eight.right - slot_eight.left) / 2,
                    (slot_eight.bottom - slot_eight.top) / 2) ==
           (COLORREF)deck_ui_rgb_to_colorref(0x12161Bu));

    /* The selected indicator and independent keyboard-focus ring stay visible. */
    assert(pixel_at(memory_dc, &slot_seven, 13,
                    (slot_seven.bottom - slot_seven.top) / 2) ==
           (COLORREF)deck_ui_rgb_to_colorref(0x9DFF00u));
    assert(pixel_at(memory_dc, &slot_zero, 5,
                    (slot_zero.bottom - slot_zero.top) / 2) ==
           (COLORREF)deck_ui_rgb_to_colorref(0xFFFFFFu));

    {
        DeckUiGridFrame invalid_frame = frame;

        invalid_frame.slots[1].bounds.x = invalid_frame.slots[0].bounds.x;
        assert(deck_ui_win32_grid_renderer_paint(
                   &renderer, memory_dc, &invalid_frame, labels, icons) ==
               DECK_UI_WIN32_RENDER_INVALID_FRAME);

        invalid_frame = frame;
        invalid_frame.slots[0].style.flags = DECK_UI_STYLE_NONE;
        assert(deck_ui_win32_grid_renderer_paint(
                   &renderer, memory_dc, &invalid_frame, labels, icons) ==
               DECK_UI_WIN32_RENDER_INVALID_FRAME);

        invalid_frame = frame;
        invalid_frame.layout_mode = DECK_UI_LAYOUT_MODE_COMPACT;
        assert(deck_ui_win32_grid_renderer_paint(
                   &renderer, memory_dc, &invalid_frame, labels, icons) ==
               DECK_UI_WIN32_RENDER_INVALID_FRAME);
    }

    frame.slot_count = DECK_UI_GRID_SLOT_COUNT - 1u;
    assert(deck_ui_win32_grid_renderer_paint(&renderer, memory_dc,
                                              &frame, labels, icons) ==
           DECK_UI_WIN32_RENDER_INVALID_FRAME);
    frame.slot_count = DECK_UI_GRID_SLOT_COUNT;

    resources_after_paint = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(resources_after_paint == resources_after_warmup);
    assert(deck_ui_win32_grid_renderer_destroy(&renderer) == 0u);
    resources_after_destroy = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(resources_after_destroy <= resources_after_paint);
    printf("GDI objects: baseline=%lu init=%lu warmup=%lu repeated=%lu destroyed=%lu\n",
           (unsigned long)resources_before_init,
           (unsigned long)resources_after_init,
           (unsigned long)resources_after_warmup,
           (unsigned long)resources_after_paint,
           (unsigned long)resources_after_destroy);

    assert(SelectObject(memory_dc, previous_bitmap) != HGDI_ERROR);
    assert(DeleteObject(bitmap));
    assert(DeleteDC(memory_dc));
    assert(ReleaseDC(NULL, screen_dc) == 1);

    puts("ui_win32_grid_renderer_test: offscreen paint, DPI, focus, GDI lifetime OK");
    return 0;
}
