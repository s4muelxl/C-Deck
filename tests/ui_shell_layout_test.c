#include "../src/ui_shell_layout.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_horizontal_tabs_keep_preferred_widths(void) {
    const int32_t widths[] = {100, 120, 100, 90, 110};
    DeckUiShellAxisInput input = {
        {20, 4, 600, 52}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        5u, widths, 64, 8, 2u
    };
    DeckUiShellAxisLayout layout;

    memset(&layout, 0xA5, sizeof(layout));
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.item_count == 5u);
    assert(layout.active_index == 2u);
    assert(layout.compressed == 0);
    assert(layout.total_extent_dip == 552);
    assert(layout.items[0].x == 20 && layout.items[0].width == 100);
    assert(layout.items[1].x == 128 && layout.items[1].width == 120);
    assert(layout.items[2].x == 256 && layout.items[2].width == 100);
    assert(layout.items[4].x == 462 && layout.items[4].width == 110);
    assert(layout.items[0].y == 4 && layout.items[0].height == 52);
}

static void test_horizontal_tabs_compress_deterministically(void) {
    const int32_t widths[] = {100, 120, 100, 90, 110};
    DeckUiShellAxisInput input = {
        {0, 0, 360, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        5u, widths, 60, 8, 1u
    };
    DeckUiShellAxisLayout first;
    DeckUiShellAxisLayout second;
    int32_t sum = 0;
    uint32_t index;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    assert(deck_ui_shell_axis_layout_compute(&input, &first) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(deck_ui_shell_axis_layout_compute(&input, &second) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(first.compressed == 1);
    assert(first.total_extent_dip == 360);
    assert(memcmp(&first, &second, sizeof(first)) == 0);
    for (index = 0u; index < first.item_count; ++index) {
        assert(first.items[index].width >= 60);
        assert(first.items[index].width <= widths[index]);
        sum += first.items[index].width;
    }
    assert(sum + 4 * 8 == 360);
    assert(first.items[0].x == 0);
    assert(first.items[4].x + first.items[4].width == 360);
}

static void test_vertical_sidebar_rows(void) {
    const int32_t heights[] = {44, 44, 52, 44};
    DeckUiShellAxisInput input = {
        {-120, 64, 216, 220}, DECK_UI_SHELL_AXIS_VERTICAL,
        4u, heights, 36, 8, DECK_UI_SHELL_NO_ACTIVE_ITEM
    };
    DeckUiShellAxisLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.active_index == DECK_UI_SHELL_NO_ACTIVE_ITEM);
    assert(layout.compressed == 0);
    assert(layout.items[0].x == -120 && layout.items[0].width == 216);
    assert(layout.items[0].y == 64 && layout.items[0].height == 44);
    assert(layout.items[1].y == 116 && layout.items[1].height == 44);
    assert(layout.items[2].y == 168 && layout.items[2].height == 52);
    assert(layout.items[3].y == 228 && layout.items[3].height == 44);
}

static void test_too_small_preserves_output(void) {
    const int32_t widths[] = {100, 120, 100, 90, 110};
    DeckUiShellAxisInput input = {
        {0, 0, 300, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        5u, widths, 60, 8, 0u
    };
    DeckUiShellAxisLayout layout;
    DeckUiShellAxisLayout before;

    memset(&layout, 0x5A, sizeof(layout));
    before = layout;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_TOO_SMALL);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);
}

static void test_invalid_arguments_preserve_output(void) {
    const int32_t widths[] = {80, 90};
    DeckUiShellAxisInput input = {
        {0, 0, 200, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        2u, widths, 40, 8, 1u
    };
    DeckUiShellAxisLayout layout;
    DeckUiShellAxisLayout before;

    memset(&layout, 0x3C, sizeof(layout));
    before = layout;
    input.active_index = 2u;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);

    input.active_index = 1u;
    input.preferred_extents_dip = NULL;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);

    input.preferred_extents_dip = widths;
    input.axis = (DeckUiShellAxis)99;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);
}

static void test_invalid_dimensions_and_item_limits(void) {
    const int32_t widths[] = {80};
    DeckUiShellAxisInput input = {
        {0, 0, 80, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        1u, widths, 40, 0, 0u
    };
    DeckUiShellAxisLayout layout;

    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    input.bounds.width = 0;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    input.bounds.width = 80;
    input.item_count = DECK_UI_SHELL_MAX_ITEMS + 1u;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
}

static void test_large_extents_and_overflow_boundaries(void) {
    const int32_t widths[] = {INT32_MAX, INT32_MAX};
    DeckUiShellAxisInput input = {
        {0, 0, INT32_MAX, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        2u, widths, 1, 0, DECK_UI_SHELL_NO_ACTIVE_ITEM
    };
    DeckUiShellAxisLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.compressed == 1);
    assert(layout.total_extent_dip == INT32_MAX);
    assert((int64_t)layout.items[0].width + layout.items[1].width ==
           INT32_MAX);

    input.bounds.x = INT32_MAX - 10;
    input.bounds.width = 20;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);

    input.bounds.x = 0;
    input.bounds.width = INT32_MAX;
    input.gap_dip = INT32_MAX;
    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_TOO_SMALL);
}

static void test_maximum_item_count(void) {
    int32_t widths[DECK_UI_SHELL_MAX_ITEMS];
    DeckUiShellAxisInput input;
    DeckUiShellAxisLayout layout;
    uint32_t index;

    for (index = 0u; index < DECK_UI_SHELL_MAX_ITEMS; ++index) {
        widths[index] = 24;
    }
    input.bounds.x = 0;
    input.bounds.y = 0;
    input.bounds.width = 400;
    input.bounds.height = 48;
    input.axis = DECK_UI_SHELL_AXIS_HORIZONTAL;
    input.item_count = DECK_UI_SHELL_MAX_ITEMS;
    input.preferred_extents_dip = widths;
    input.min_extent_dip = 20;
    input.gap_dip = 1;
    input.active_index = 15u;

    assert(deck_ui_shell_axis_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.item_count == DECK_UI_SHELL_MAX_ITEMS);
    assert(layout.active_index == 15u);
    assert(layout.items[15].x + layout.items[15].width <= 400);
}


static void test_responsive_shell_mode_breakpoints(void) {
    static const struct {
        int32_t width;
        DeckUiShellMode expected;
    } cases[] = {
        {1360, DECK_UI_SHELL_MODE_FULL},
        {1359, DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE},
        {1120, DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE},
        {1119, DECK_UI_SHELL_MODE_SIDEBAR_RAIL},
        {900, DECK_UI_SHELL_MODE_SIDEBAR_RAIL},
        {899, DECK_UI_SHELL_MODE_COMPACT},
        {1, DECK_UI_SHELL_MODE_COMPACT}
    };
    DeckUiShellMode mode = DECK_UI_SHELL_MODE_FULL;
    uint32_t index;

    for (index = 0u; index < (uint32_t)(sizeof(cases) / sizeof(cases[0])); ++index) {
        assert(deck_ui_shell_mode_resolve(cases[index].width, 0, &mode) ==
               DECK_UI_SHELL_MODE_OK);
        assert(mode == cases[index].expected);
    }
}

static void test_responsive_shell_mode_override_and_invalid_inputs(void) {
    DeckUiShellMode mode = DECK_UI_SHELL_MODE_SIDEBAR_RAIL;

    assert(deck_ui_shell_mode_resolve(1920, 1, &mode) == DECK_UI_SHELL_MODE_OK);
    assert(mode == DECK_UI_SHELL_MODE_COMPACT);

    mode = DECK_UI_SHELL_MODE_SIDEBAR_RAIL;
    assert(deck_ui_shell_mode_resolve(0, 0, &mode) ==
           DECK_UI_SHELL_MODE_INVALID_ARGUMENT);
    assert(mode == DECK_UI_SHELL_MODE_SIDEBAR_RAIL);

    assert(deck_ui_shell_mode_resolve(1200, 2, &mode) ==
           DECK_UI_SHELL_MODE_INVALID_ARGUMENT);
    assert(mode == DECK_UI_SHELL_MODE_SIDEBAR_RAIL);

    assert(deck_ui_shell_mode_resolve(1200, 0, NULL) ==
           DECK_UI_SHELL_MODE_INVALID_ARGUMENT);
}

static void test_shell_frame_keeps_mode_and_grid_policy_in_sync(void) {
    static const struct {
        int32_t width;
        DeckUiShellMode shell_mode;
        DeckUiLayoutMode layout_mode;
    } cases[] = {
        {899, DECK_UI_SHELL_MODE_COMPACT, DECK_UI_LAYOUT_MODE_COMPACT},
        {900, DECK_UI_SHELL_MODE_SIDEBAR_RAIL, DECK_UI_LAYOUT_MODE_NARROW},
        {1119, DECK_UI_SHELL_MODE_SIDEBAR_RAIL, DECK_UI_LAYOUT_MODE_NARROW},
        {1120, DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE, DECK_UI_LAYOUT_MODE_MEDIUM},
        {1359, DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE, DECK_UI_LAYOUT_MODE_MEDIUM},
        {1360, DECK_UI_SHELL_MODE_FULL, DECK_UI_LAYOUT_MODE_WIDE}
    };
    uint32_t index;

    for (index = 0u; index < (uint32_t)(sizeof(cases) / sizeof(cases[0])); ++index) {
        DeckUiShellFrame frame;

        memset(&frame, 0, sizeof(frame));
        assert(deck_ui_shell_frame_compute(cases[index].width, 800, 0,
                                           &frame) ==
               DECK_UI_SHELL_LAYOUT_OK);
        assert(frame.mode == cases[index].shell_mode);
        assert(frame.layout.mode == cases[index].layout_mode);
        assert(frame.layout.slot_count == DECK_UI_GRID_SLOT_COUNT);
        assert(deck_ui_layout_validate(&frame.layout) ==
               DECK_UI_LAYOUT_VALIDATION_OK);
    }
}

static void test_shell_frame_compact_override_and_failure_preservation(void) {
    DeckUiShellFrame frame;
    DeckUiShellFrame before;

    memset(&frame, 0x5A, sizeof(frame));
    assert(deck_ui_shell_frame_compute(1920, 1080, 1, &frame) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(frame.mode == DECK_UI_SHELL_MODE_COMPACT);
    assert(frame.layout.mode == DECK_UI_LAYOUT_MODE_COMPACT);
    assert(frame.layout.sidebar_visible == 0);
    assert(frame.layout.inspector_visible == 0);

    memset(&frame, 0x3C, sizeof(frame));
    before = frame;
    assert(deck_ui_shell_frame_compute(899, 100, 0, &frame) ==
           DECK_UI_SHELL_LAYOUT_TOO_SMALL);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);

    assert(deck_ui_shell_frame_compute(1920, 800, 2, &frame) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
    assert(deck_ui_shell_frame_compute(0, 800, 0, &frame) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
    assert(deck_ui_shell_frame_compute(1920, 0, 0, &frame) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&frame, &before, sizeof(frame)) == 0);
}



static void test_overflow_layout_keeps_active_item_visible(void) {
    const int32_t extents[] = {100, 120, 100, 90, 110};
    DeckUiShellOverflowInput input = {
        {10, 20, 300, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        5u, extents, 60, 8, 44, 4u
    };
    DeckUiShellOverflowLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.overflow_visible == 1);
    assert(layout.visible_count == 3u);
    assert(layout.hidden_count == 2u);
    assert(layout.visible_indices[0] == 0u);
    assert(layout.visible_indices[1] == 1u);
    assert(layout.visible_indices[2] == 4u);
    assert(layout.hidden_indices[0] == 2u);
    assert(layout.hidden_indices[1] == 3u);
    assert(layout.visible_layout.active_index == 2u);
    assert(layout.visible_layout.total_extent_dip == 248);
    assert(layout.overflow_rect.x == 266);
    assert(layout.overflow_rect.width == 44);
    assert(layout.total_extent_dip == 300);
}

static void test_overflow_layout_without_active_preserves_first_items(void) {
    const int32_t extents[] = {70, 80, 90, 100, 110};
    DeckUiShellOverflowInput input = {
        {-40, 8, 180, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        5u, extents, 44, 8, 44, DECK_UI_SHELL_NO_ACTIVE_ITEM
    };
    DeckUiShellOverflowLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.overflow_visible == 1);
    assert(layout.visible_count == 2u);
    assert(layout.hidden_count == 3u);
    assert(layout.visible_indices[0] == 0u);
    assert(layout.visible_indices[1] == 1u);
    assert(layout.visible_layout.active_index == DECK_UI_SHELL_NO_ACTIVE_ITEM);
    assert(layout.overflow_rect.x == 96);
    assert(layout.overflow_rect.x + layout.overflow_rect.width == 140);
}

static void test_overflow_layout_uses_compression_before_hiding_items(void) {
    const int32_t extents[] = {100, 100};
    DeckUiShellOverflowInput input = {
        {0, 0, 150, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        2u, extents, 60, 8, 44, 1u
    };
    DeckUiShellOverflowLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.overflow_visible == 0);
    assert(layout.visible_count == 2u);
    assert(layout.hidden_count == 0u);
    assert(layout.visible_layout.compressed == 1);
    assert(layout.visible_layout.active_index == 1u);
    assert(layout.overflow_rect.width == 0);
}

static void test_vertical_overflow_is_pinned_to_bottom(void) {
    const int32_t extents[] = {60, 70, 80, 90};
    DeckUiShellOverflowInput input = {
        {-120, 10, 216, 180}, DECK_UI_SHELL_AXIS_VERTICAL,
        4u, extents, 44, 8, 44, 3u
    };
    DeckUiShellOverflowLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.overflow_visible == 1);
    assert(layout.visible_count == 2u);
    assert(layout.visible_indices[0] == 0u);
    assert(layout.visible_indices[1] == 3u);
    assert(layout.hidden_count == 2u);
    assert(layout.visible_layout.active_index == 1u);
    assert(layout.overflow_rect.y == 146);
    assert(layout.overflow_rect.height == 44);
    assert(layout.total_extent_dip == 180);
}

static void test_single_item_never_creates_overflow(void) {
    const int32_t extents[] = {100};
    DeckUiShellOverflowInput input = {
        {0, 0, 80, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        1u, extents, 44, 8, 44, 0u
    };
    DeckUiShellOverflowLayout layout;

    memset(&layout, 0, sizeof(layout));
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_OK);
    assert(layout.overflow_visible == 0);
    assert(layout.visible_count == 1u);
    assert(layout.hidden_count == 0u);
    assert(layout.visible_layout.items[0].width == 80);
}

static void test_overflow_too_small_preserves_output(void) {
    const int32_t extents[] = {80, 80};
    DeckUiShellOverflowInput input = {
        {0, 0, 100, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        2u, extents, 60, 8, 44, 0u
    };
    DeckUiShellOverflowLayout layout;
    DeckUiShellOverflowLayout before;

    memset(&layout, 0x5A, sizeof(layout));
    before = layout;
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_TOO_SMALL);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);
}

static void test_overflow_invalid_arguments_preserve_output(void) {
    const int32_t extents[] = {80, 90};
    DeckUiShellOverflowInput input = {
        {0, 0, 200, 48}, DECK_UI_SHELL_AXIS_HORIZONTAL,
        2u, extents, 40, 8, 44, 1u
    };
    DeckUiShellOverflowLayout layout;
    DeckUiShellOverflowLayout before;

    memset(&layout, 0x3C, sizeof(layout));
    before = layout;
    input.overflow_extent_dip = 0;
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);

    input.overflow_extent_dip = 44;
    input.active_index = 2u;
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);

    input.active_index = DECK_UI_SHELL_NO_ACTIVE_ITEM;
    input.bounds.width = 0;
    assert(deck_ui_shell_overflow_layout_compute(&input, &layout) ==
           DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT);
    assert(memcmp(&layout, &before, sizeof(layout)) == 0);
}

int main(void) {
    test_horizontal_tabs_keep_preferred_widths();
    test_horizontal_tabs_compress_deterministically();
    test_vertical_sidebar_rows();
    test_too_small_preserves_output();
    test_invalid_arguments_preserve_output();
    test_invalid_dimensions_and_item_limits();
    test_large_extents_and_overflow_boundaries();
    test_maximum_item_count();
    test_responsive_shell_mode_breakpoints();
    test_responsive_shell_mode_override_and_invalid_inputs();
    test_shell_frame_keeps_mode_and_grid_policy_in_sync();
    test_shell_frame_compact_override_and_failure_preservation();
    test_overflow_layout_keeps_active_item_visible();
    test_overflow_layout_without_active_preserves_first_items();
    test_overflow_layout_uses_compression_before_hiding_items();
    test_vertical_overflow_is_pinned_to_bottom();
    test_single_item_never_creates_overflow();
    test_overflow_too_small_preserves_output();
    test_overflow_invalid_arguments_preserve_output();
    puts("ui_shell_layout_test: all tests passed");
    return 0;
}
