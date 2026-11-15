#include "../src/ui_layout.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void assert_grid_is_valid(const DeckUiLayout *layout) {
    uint32_t index;

    assert(layout->slot_count == DECK_UI_GRID_SLOT_COUNT);
    assert(layout->slot_size_dip >= 44);
    assert(layout->slot_gap_dip >= 0);

    for (index = 0u; index < layout->slot_count; ++index) {
        const DeckUiRect *slot = &layout->slots[index];
        assert(slot->width == layout->slot_size_dip);
        assert(slot->height == layout->slot_size_dip);
        assert(slot->x >= layout->workspace.x);
        assert(slot->y >= layout->workspace.y);
        assert(slot->x + slot->width <= layout->workspace.x + layout->workspace.width);
        assert(slot->y + slot->height <= layout->workspace.y + layout->workspace.height);
    }

    for (index = 0u; index < layout->slot_count; ++index) {
        uint32_t other;
        for (other = index + 1u; other < layout->slot_count; ++other) {
            const DeckUiRect *a = &layout->slots[index];
            const DeckUiRect *b = &layout->slots[other];
            const int separated = a->x + a->width <= b->x
                                  || b->x + b->width <= a->x
                                  || a->y + a->height <= b->y
                                  || b->y + b->height <= a->y;
            assert(separated);
        }
    }
}

static void test_wide_layout(void) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    DeckUiLayout layout;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    assert(layout.mode == DECK_UI_LAYOUT_MODE_WIDE);
    assert(layout.sidebar_visible == 1);
    assert(layout.inspector_visible == 1);
    assert(layout.sidebar.width == 216);
    assert(layout.inspector.width == 304);
    assert(layout.workspace.x == 216);
    assert(layout.workspace.width == 1080);
    assert(layout.top_bar.height == 64);
    assert(layout.status_bar.height == 34);
    assert_grid_is_valid(&layout);
}

static void test_medium_and_narrow_layouts(void) {
    const DeckUiLayoutInput medium_input = { 1200, 800, 0 };
    const DeckUiLayoutInput narrow_input = { 1000, 720, 0 };
    DeckUiLayout medium;
    DeckUiLayout narrow;

    assert(deck_ui_layout_compute(&medium_input, &medium) == DECK_UI_LAYOUT_OK);
    assert(medium.mode == DECK_UI_LAYOUT_MODE_MEDIUM);
    assert(medium.sidebar_visible == 1);
    assert(medium.sidebar.width == 216);
    assert(medium.inspector_visible == 0);
    assert(medium.inspector.width == 0);
    assert_grid_is_valid(&medium);

    assert(deck_ui_layout_compute(&narrow_input, &narrow) == DECK_UI_LAYOUT_OK);
    assert(narrow.mode == DECK_UI_LAYOUT_MODE_NARROW);
    assert(narrow.sidebar_visible == 1);
    assert(narrow.sidebar.width == 64);
    assert(narrow.inspector_visible == 0);
    assert_grid_is_valid(&narrow);
}

static void test_compact_minimum_layout(void) {
    const DeckUiLayoutInput input = { 440, 320, 1 };
    DeckUiLayout layout;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    assert(layout.mode == DECK_UI_LAYOUT_MODE_COMPACT);
    assert(layout.sidebar_visible == 0);
    assert(layout.inspector_visible == 0);
    assert(layout.top_bar.height == 48);
    assert(layout.status_bar.height == 30);
    assert(layout.slot_gap_dip == 8);
    assert_grid_is_valid(&layout);
}

static void test_unsupported_and_too_short_layouts(void) {
    const DeckUiLayoutInput too_narrow = { 899, 640, 0 };
    const DeckUiLayoutInput too_short = { 1000, 120, 0 };
    DeckUiLayout layout;

    assert(deck_ui_layout_compute(&too_narrow, &layout) == DECK_UI_LAYOUT_TOO_SMALL);
    assert(layout.mode == DECK_UI_LAYOUT_MODE_UNSUPPORTED);
    assert(layout.slot_count == 0u);

    assert(deck_ui_layout_compute(&too_short, &layout) == DECK_UI_LAYOUT_TOO_SMALL);
    assert(layout.slot_count == 0u);
}

static void test_invalid_arguments(void) {
    const DeckUiLayoutInput zero_width = { 0, 640, 0 };
    DeckUiLayout layout;

    memset(&layout, 0xA5, sizeof(layout));
    assert(deck_ui_layout_compute(&zero_width, &layout) == DECK_UI_LAYOUT_INVALID_ARGUMENT);
    assert(layout.mode == DECK_UI_LAYOUT_MODE_UNSUPPORTED);
    assert(layout.slot_count == 0u);
    assert(deck_ui_layout_compute(NULL, &layout) == DECK_UI_LAYOUT_INVALID_ARGUMENT);
    assert(deck_ui_layout_compute(&zero_width, NULL) == DECK_UI_LAYOUT_INVALID_ARGUMENT);
}

static void test_dpi_conversion(void) {
    assert(deck_ui_dip_to_px(10, 96u) == 10);
    assert(deck_ui_dip_to_px(10, 120u) == 13);
    assert(deck_ui_dip_to_px(10, 144u) == 15);
    assert(deck_ui_dip_to_px(10, 192u) == 20);
    assert(deck_ui_dip_to_px(1, 144u) == 2);
    assert(deck_ui_dip_to_px(-1, 144u) == -2);
    assert(deck_ui_dip_to_px(10, 0u) == 10);
    assert(deck_ui_dip_to_px(INT32_MAX, UINT32_MAX) == INT32_MAX);
    assert(deck_ui_dip_to_px(INT32_MIN, UINT32_MAX) == INT32_MIN);

    assert(deck_ui_px_to_dip(10, 96u) == 10);
    assert(deck_ui_px_to_dip(13, 120u) == 10);
    assert(deck_ui_px_to_dip(15, 144u) == 10);
    assert(deck_ui_px_to_dip(20, 192u) == 10);
    assert(deck_ui_px_to_dip(1, 144u) == 1);
    assert(deck_ui_px_to_dip(-1, 144u) == -1);
    assert(deck_ui_px_to_dip(10, 0u) == 10);
    assert(deck_ui_px_to_dip(INT32_MAX, 1u) == INT32_MAX);
    assert(deck_ui_px_to_dip(INT32_MIN, 1u) == INT32_MIN);
}

int main(void) {
    test_wide_layout();
    test_medium_and_narrow_layouts();
    test_compact_minimum_layout();
    test_unsupported_and_too_short_layouts();
    test_invalid_arguments();
    test_dpi_conversion();
    puts("ui_layout_test: all checks passed");
    return 0;
}
