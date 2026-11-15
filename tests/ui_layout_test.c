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

static void assert_rect_is_zero(const DeckUiRect *rect) {
    assert(rect->x == 0);
    assert(rect->y == 0);
    assert(rect->width == 0);
    assert(rect->height == 0);
}

static void assert_layout_is_empty(const DeckUiLayout *layout) {
    uint32_t index;

    assert(layout->mode == DECK_UI_LAYOUT_MODE_UNSUPPORTED);
    assert_rect_is_zero(&layout->top_bar);
    assert_rect_is_zero(&layout->sidebar);
    assert_rect_is_zero(&layout->workspace);
    assert_rect_is_zero(&layout->inspector);
    assert_rect_is_zero(&layout->status_bar);
    assert(layout->slot_size_dip == 0);
    assert(layout->slot_gap_dip == 0);
    assert(layout->slot_count == 0u);
    assert(layout->sidebar_visible == 0);
    assert(layout->inspector_visible == 0);

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        assert_rect_is_zero(&layout->slots[index]);
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

static void test_too_small_outputs_are_empty(void) {
    static const DeckUiLayoutInput inputs[] = {
        { 899, 640, 0 },  /* below the normal breakpoint */
        { 1000, 120, 0 }, /* chrome leaves no usable workspace height */
        { 900, 250, 0 },  /* grid cannot preserve the 44 DIP key target */
        { 280, 320, 1 },  /* compact width is insufficient */
        { 440, 78, 1 }    /* compact chrome consumes the full height */
    };
    size_t index;

    for (index = 0u; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        DeckUiLayout layout;
        memset(&layout, 0xA5, sizeof(layout));
        assert(deck_ui_layout_compute(&inputs[index], &layout) == DECK_UI_LAYOUT_TOO_SMALL);
        assert_layout_is_empty(&layout);
    }
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



static void assert_rect_equal(const DeckUiRect *actual,
                               const DeckUiRect *expected) {
    assert(actual->x == expected->x);
    assert(actual->y == expected->y);
    assert(actual->width == expected->width);
    assert(actual->height == expected->height);
}

static void test_rect_scaling_preserves_shared_edges(void) {
    const DeckUiRect first_dip = { 1, 0, 1, 10 };
    const DeckUiRect second_dip = { 2, 0, 1, 10 };
    DeckUiRect first_px;
    DeckUiRect second_px;

    assert(deck_ui_rect_dip_to_px(&first_dip, 120u, &first_px)
           == DECK_UI_RECT_SCALE_OK);
    assert(deck_ui_rect_dip_to_px(&second_dip, 120u, &second_px)
           == DECK_UI_RECT_SCALE_OK);

    /* 1 DIP at x=1 spans two pixels at 125%; the next rect starts exactly there. */
    assert(first_px.x == 1);
    assert(first_px.width == 2);
    assert(second_px.x == first_px.x + first_px.width);
}

static void test_rect_scaling_standard_dpi_values(void) {
    const DeckUiRect input = { 10, 20, 40, 50 };
    const DeckUiRect expected_96 = { 10, 20, 40, 50 };
    const DeckUiRect expected_120 = { 13, 25, 50, 63 };
    const DeckUiRect expected_144 = { 15, 30, 60, 75 };
    const DeckUiRect expected_192 = { 20, 40, 80, 100 };
    DeckUiRect actual;

    assert(deck_ui_rect_dip_to_px(&input, 96u, &actual) == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_96);
    assert(deck_ui_rect_dip_to_px(&input, 120u, &actual) == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_120);
    assert(deck_ui_rect_dip_to_px(&input, 144u, &actual) == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_144);
    assert(deck_ui_rect_dip_to_px(&input, 192u, &actual) == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_192);

    assert(deck_ui_rect_px_to_dip(&expected_120, 120u, &actual)
           == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &input);
}

static void test_rect_scaling_negative_origins_and_zero_area(void) {
    const DeckUiRect negative_origin = { -101, -51, 101, 51 };
    const DeckUiRect expected_negative = { -126, -64, 126, 64 };
    const DeckUiRect zero_area = { -5, 7, 0, 0 };
    const DeckUiRect expected_zero_area = { -8, 11, 0, 0 };
    DeckUiRect actual;

    assert(deck_ui_rect_dip_to_px(&negative_origin, 120u, &actual)
           == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_negative);

    assert(deck_ui_rect_dip_to_px(&zero_area, 144u, &actual)
           == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &expected_zero_area);

    assert(deck_ui_rect_dip_to_px(&zero_area, 0u, &actual)
           == DECK_UI_RECT_SCALE_OK);
    assert_rect_equal(&actual, &zero_area);
}

static void test_rect_scaling_rejects_invalid_and_overflow_without_mutation(void) {
    const DeckUiRect negative_width = { 1, 2, -1, 4 };
    const DeckUiRect overflow_edge = { INT32_MAX, 0, 1, 1 };
    const DeckUiRect overflow_scale = { INT32_MAX, 0, 0, 1 };
    const DeckUiRect expected_unchanged = { 9, 8, 7, 6 };
    DeckUiRect output = expected_unchanged;

    assert(deck_ui_rect_dip_to_px(&negative_width, 120u, &output)
           == DECK_UI_RECT_SCALE_INVALID_ARGUMENT);
    assert_rect_equal(&output, &expected_unchanged);

    assert(deck_ui_rect_dip_to_px(&overflow_edge, 96u, &output)
           == DECK_UI_RECT_SCALE_OUT_OF_RANGE);
    assert_rect_equal(&output, &expected_unchanged);

    assert(deck_ui_rect_dip_to_px(&overflow_scale, UINT32_MAX, &output)
           == DECK_UI_RECT_SCALE_OUT_OF_RANGE);
    assert_rect_equal(&output, &expected_unchanged);

    assert(deck_ui_rect_dip_to_px(NULL, 96u, &output)
           == DECK_UI_RECT_SCALE_INVALID_ARGUMENT);
    assert(deck_ui_rect_dip_to_px(&expected_unchanged, 96u, NULL)
           == DECK_UI_RECT_SCALE_INVALID_ARGUMENT);
    assert_rect_equal(&output, &expected_unchanged);
}

static void test_grid_hit_test_all_slots(void) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    DeckUiLayout layout;
    uint32_t index;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiRect *slot = &layout.slots[index];
        const int32_t center_x = slot->x + slot->width / 2;
        const int32_t center_y = slot->y + slot->height / 2;
        uint32_t hit = DECK_UI_GRID_SLOT_COUNT;

        assert(deck_ui_grid_hit_test(&layout, center_x, center_y, &hit)
               == DECK_UI_HIT_TEST_HIT);
        assert(hit == index);
    }
}

static void test_grid_hit_test_uses_half_open_bounds(void) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    DeckUiLayout layout;
    const DeckUiRect *first;
    uint32_t hit = 77u;
    int32_t center_y;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    first = &layout.slots[0];
    center_y = first->y + first->height / 2;

    assert(deck_ui_grid_hit_test(&layout, first->x, first->y, &hit)
           == DECK_UI_HIT_TEST_HIT);
    assert(hit == 0u);

    hit = 77u;
    assert(deck_ui_grid_hit_test(&layout,
                                 first->x + first->width - 1,
                                 center_y,
                                 &hit) == DECK_UI_HIT_TEST_HIT);
    assert(hit == 0u);

    hit = 77u;
    assert(deck_ui_grid_hit_test(&layout,
                                 first->x + first->width,
                                 center_y,
                                 &hit) == DECK_UI_HIT_TEST_OUTSIDE);
    assert(hit == 77u);

    assert(deck_ui_grid_hit_test(&layout, 0, 0, &hit)
           == DECK_UI_HIT_TEST_OUTSIDE);
    assert(hit == 77u);
}

static void test_grid_hit_test_rejects_invalid_layout(void) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    DeckUiLayout layout;
    uint32_t hit = 77u;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    layout.slots[1].x = layout.slots[0].x;
    assert(deck_ui_grid_hit_test(&layout, layout.slots[0].x,
                                 layout.slots[0].y, &hit)
           == DECK_UI_HIT_TEST_INVALID_LAYOUT);
    assert(hit == 77u);

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    layout.slots[0].width = 0;
    assert(deck_ui_grid_hit_test(&layout, 10, 10, &hit)
           == DECK_UI_HIT_TEST_INVALID_LAYOUT);
    assert(hit == 77u);
}

static void test_grid_hit_test_invalid_arguments(void) {
    const DeckUiLayoutInput input = { 1600, 900, 0 };
    DeckUiLayout layout;
    uint32_t hit = 77u;

    assert(deck_ui_layout_compute(&input, &layout) == DECK_UI_LAYOUT_OK);
    assert(deck_ui_grid_hit_test(NULL, 0, 0, &hit)
           == DECK_UI_HIT_TEST_INVALID_ARGUMENT);
    assert(deck_ui_grid_hit_test(&layout, 0, 0, NULL)
           == DECK_UI_HIT_TEST_INVALID_ARGUMENT);
    assert(hit == 77u);
}

int main(void) {
    test_wide_layout();
    test_medium_and_narrow_layouts();
    test_compact_minimum_layout();
    test_too_small_outputs_are_empty();
    test_invalid_arguments();
    test_dpi_conversion();
    test_rect_scaling_preserves_shared_edges();
    test_rect_scaling_standard_dpi_values();
    test_rect_scaling_negative_origins_and_zero_area();
    test_rect_scaling_rejects_invalid_and_overflow_without_mutation();
    test_grid_hit_test_all_slots();
    test_grid_hit_test_uses_half_open_bounds();
    test_grid_hit_test_rejects_invalid_layout();
    test_grid_hit_test_invalid_arguments();
    puts("ui_layout_test: all checks passed");
    return 0;
}
