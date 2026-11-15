#include "ui_layout.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

enum {
    NORMAL_TOP_BAR_DIP = 64,
    NORMAL_STATUS_BAR_DIP = 34,
    COMPACT_TOP_BAR_DIP = 48,
    COMPACT_STATUS_BAR_DIP = 30,
    SIDEBAR_FULL_DIP = 216,
    SIDEBAR_RAIL_DIP = 64,
    INSPECTOR_DIP = 304,
    GRID_PADDING_DIP = 20,
    GRID_GAP_DIP = 14,
    COMPACT_GRID_GAP_DIP = 8,
    MIN_SLOT_DIP = 44,
    MAX_SLOT_DIP = 108,
    NARROW_BREAKPOINT_DIP = 900,
    MEDIUM_BREAKPOINT_DIP = 1120,
    WIDE_BREAKPOINT_DIP = 1360
};

static int32_t min_i32(int32_t left, int32_t right) {
    return left < right ? left : right;
}

static void set_rect(DeckUiRect *rect,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height) {
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
}

static DeckUiLayoutStatus too_small(DeckUiLayout *layout) {
    /*
     * Fail closed: callers must never consume partially computed regions
     * after a TOO_SMALL result. Keep the output deterministic and empty.
     */
    memset(layout, 0, sizeof(*layout));
    layout->mode = DECK_UI_LAYOUT_MODE_UNSUPPORTED;
    return DECK_UI_LAYOUT_TOO_SMALL;
}

DeckUiLayoutStatus deck_ui_layout_compute(const DeckUiLayoutInput *input,
                                          DeckUiLayout *layout) {
    int32_t top_height;
    int32_t status_height;
    int32_t sidebar_width = 0;
    int32_t inspector_width = 0;
    int32_t body_height;
    int32_t available_width;
    int32_t available_height;
    int32_t slot_size;
    int32_t gap;
    int32_t grid_width;
    int32_t grid_height;
    int32_t grid_x;
    int32_t grid_y;
    int32_t row;
    int32_t column;

    if (!layout) return DECK_UI_LAYOUT_INVALID_ARGUMENT;
    memset(layout, 0, sizeof(*layout));
    layout->mode = DECK_UI_LAYOUT_MODE_UNSUPPORTED;

    if (!input || input->width_dip <= 0 || input->height_dip <= 0) {
        return DECK_UI_LAYOUT_INVALID_ARGUMENT;
    }

    if (input->compact_mode) {
        layout->mode = DECK_UI_LAYOUT_MODE_COMPACT;
        top_height = COMPACT_TOP_BAR_DIP;
        status_height = COMPACT_STATUS_BAR_DIP;
        gap = COMPACT_GRID_GAP_DIP;
    } else {
        if (input->width_dip < NARROW_BREAKPOINT_DIP) {
            return too_small(layout);
        }
        top_height = NORMAL_TOP_BAR_DIP;
        status_height = NORMAL_STATUS_BAR_DIP;
        gap = GRID_GAP_DIP;

        if (input->width_dip >= WIDE_BREAKPOINT_DIP) {
            layout->mode = DECK_UI_LAYOUT_MODE_WIDE;
            sidebar_width = SIDEBAR_FULL_DIP;
            inspector_width = INSPECTOR_DIP;
        } else if (input->width_dip >= MEDIUM_BREAKPOINT_DIP) {
            layout->mode = DECK_UI_LAYOUT_MODE_MEDIUM;
            sidebar_width = SIDEBAR_FULL_DIP;
        } else {
            layout->mode = DECK_UI_LAYOUT_MODE_NARROW;
            sidebar_width = SIDEBAR_RAIL_DIP;
        }
    }

    if (input->height_dip <= top_height + status_height) {
        return too_small(layout);
    }

    body_height = input->height_dip - top_height - status_height;
    set_rect(&layout->top_bar, 0, 0, input->width_dip, top_height);
    set_rect(&layout->status_bar, 0, input->height_dip - status_height,
             input->width_dip, status_height);

    layout->sidebar_visible = sidebar_width > 0;
    layout->inspector_visible = inspector_width > 0;

    if (layout->sidebar_visible) {
        set_rect(&layout->sidebar, 0, top_height, sidebar_width, body_height);
    }
    if (layout->inspector_visible) {
        set_rect(&layout->inspector,
                 input->width_dip - inspector_width,
                 top_height,
                 inspector_width,
                 body_height);
    }

    if (input->width_dip <= sidebar_width + inspector_width) {
        return too_small(layout);
    }

    set_rect(&layout->workspace,
             sidebar_width,
             top_height,
             input->width_dip - sidebar_width - inspector_width,
             body_height);

    available_width = layout->workspace.width - 2 * GRID_PADDING_DIP
                      - (DECK_UI_GRID_COLUMNS - 1) * gap;
    available_height = layout->workspace.height - 2 * GRID_PADDING_DIP
                       - (DECK_UI_GRID_ROWS - 1) * gap;

    if (available_width <= 0 || available_height <= 0) {
        return too_small(layout);
    }

    slot_size = min_i32(available_width / DECK_UI_GRID_COLUMNS,
                        available_height / DECK_UI_GRID_ROWS);
    if (slot_size < MIN_SLOT_DIP) {
        return too_small(layout);
    }
    /* Preserve the native key-size ceiling on large and high-DPI workspaces. */
    if (slot_size > MAX_SLOT_DIP) slot_size = MAX_SLOT_DIP;

    grid_width = DECK_UI_GRID_COLUMNS * slot_size
                 + (DECK_UI_GRID_COLUMNS - 1) * gap;
    grid_height = DECK_UI_GRID_ROWS * slot_size
                  + (DECK_UI_GRID_ROWS - 1) * gap;
    grid_x = layout->workspace.x + (layout->workspace.width - grid_width) / 2;
    grid_y = layout->workspace.y + (layout->workspace.height - grid_height) / 2;

    layout->slot_size_dip = slot_size;
    layout->slot_gap_dip = gap;

    for (row = 0; row < DECK_UI_GRID_ROWS; ++row) {
        for (column = 0; column < DECK_UI_GRID_COLUMNS; ++column) {
            const int32_t index = row * DECK_UI_GRID_COLUMNS + column;
            const int32_t x = grid_x + column * (slot_size + gap);
            const int32_t y = grid_y + row * (slot_size + gap);
            set_rect(&layout->slots[index], x, y, slot_size, slot_size);
        }
    }

    layout->slot_count = DECK_UI_GRID_SLOT_COUNT;
    return DECK_UI_LAYOUT_OK;
}

int32_t deck_ui_dip_to_px(int32_t dip, uint32_t dpi) {
    int64_t scaled;
    int64_t pixels;

    if (dpi == 0u) dpi = 96u;
    scaled = (int64_t)dip * (int64_t)dpi;
    scaled += scaled >= 0 ? 48 : -48;
    pixels = scaled / 96;

    if (pixels > INT32_MAX) return INT32_MAX;
    if (pixels < INT32_MIN) return INT32_MIN;
    return (int32_t)pixels;
}


int32_t deck_ui_px_to_dip(int32_t pixels, uint32_t dpi) {
    int64_t scaled;
    int64_t dips;

    if (dpi == 0u) dpi = 96u;
    scaled = (int64_t)pixels * 96;
    scaled += scaled >= 0 ? (int64_t)dpi / 2 : -((int64_t)dpi / 2);
    dips = scaled / (int64_t)dpi;

    if (dips > INT32_MAX) return INT32_MAX;
    if (dips < INT32_MIN) return INT32_MIN;
    return (int32_t)dips;
}


static int scale_coordinate_checked(int64_t coordinate,
                                     uint32_t numerator,
                                     uint32_t denominator,
                                     int32_t *result) {
    int64_t scaled;
    int64_t rounding;

    if (!result || denominator == 0u) return 0;

    /*
     * The input edge is constrained to int32, so multiplying by a uint32
     * scale factor fits in int64. Round halves away from zero, matching the
     * scalar DIP/pixel conversion helpers above.
     */
    scaled = coordinate * (int64_t)numerator;
    rounding = (int64_t)denominator / 2;
    scaled += scaled >= 0 ? rounding : -rounding;
    scaled /= (int64_t)denominator;

    if (scaled > INT32_MAX || scaled < INT32_MIN) return 0;
    *result = (int32_t)scaled;
    return 1;
}

static DeckUiRectScaleStatus scale_rect_edges(const DeckUiRect *source,
                                               uint32_t numerator,
                                               uint32_t denominator,
                                               DeckUiRect *destination) {
    DeckUiRect converted;
    int64_t right;
    int64_t bottom;
    int64_t width;
    int64_t height;

    if (!source || !destination || source->width < 0 || source->height < 0) {
        return DECK_UI_RECT_SCALE_INVALID_ARGUMENT;
    }

    right = (int64_t)source->x + source->width;
    bottom = (int64_t)source->y + source->height;
    if (right > INT32_MAX || right < INT32_MIN
        || bottom > INT32_MAX || bottom < INT32_MIN) {
        return DECK_UI_RECT_SCALE_OUT_OF_RANGE;
    }

    if (!scale_coordinate_checked(source->x, numerator, denominator,
                                  &converted.x)
        || !scale_coordinate_checked(source->y, numerator, denominator,
                                     &converted.y)) {
        return DECK_UI_RECT_SCALE_OUT_OF_RANGE;
    }

    {
        int32_t right_scaled;
        int32_t bottom_scaled;

        if (!scale_coordinate_checked(right, numerator, denominator,
                                      &right_scaled)
            || !scale_coordinate_checked(bottom, numerator, denominator,
                                         &bottom_scaled)) {
            return DECK_UI_RECT_SCALE_OUT_OF_RANGE;
        }

        width = (int64_t)right_scaled - converted.x;
        height = (int64_t)bottom_scaled - converted.y;
    }

    if (width < 0 || width > INT32_MAX
        || height < 0 || height > INT32_MAX) {
        return DECK_UI_RECT_SCALE_OUT_OF_RANGE;
    }

    converted.width = (int32_t)width;
    converted.height = (int32_t)height;
    *destination = converted;
    return DECK_UI_RECT_SCALE_OK;
}

DeckUiRectScaleStatus deck_ui_rect_dip_to_px(const DeckUiRect *dip_rect,
                                             uint32_t dpi,
                                             DeckUiRect *pixel_rect) {
    if (dpi == 0u) dpi = 96u;
    return scale_rect_edges(dip_rect, dpi, 96u, pixel_rect);
}

DeckUiRectScaleStatus deck_ui_rect_px_to_dip(const DeckUiRect *pixel_rect,
                                             uint32_t dpi,
                                             DeckUiRect *dip_rect) {
    if (dpi == 0u) dpi = 96u;
    return scale_rect_edges(pixel_rect, 96u, dpi, dip_rect);
}


static int layout_grid_is_valid(const DeckUiLayout *layout) {
    int64_t workspace_right;
    int64_t workspace_bottom;
    uint32_t index;

    if (!layout || layout->mode < DECK_UI_LAYOUT_MODE_WIDE
        || layout->mode > DECK_UI_LAYOUT_MODE_COMPACT
        || layout->slot_count != DECK_UI_GRID_SLOT_COUNT
        || layout->slot_size_dip < MIN_SLOT_DIP
        || layout->slot_size_dip > MAX_SLOT_DIP
        || layout->slot_gap_dip < 0
        || layout->workspace.width <= 0
        || layout->workspace.height <= 0) {
        return 0;
    }

    workspace_right = (int64_t)layout->workspace.x + layout->workspace.width;
    workspace_bottom = (int64_t)layout->workspace.y + layout->workspace.height;
    if (workspace_right > INT32_MAX || workspace_right < INT32_MIN
        || workspace_bottom > INT32_MAX || workspace_bottom < INT32_MIN) {
        return 0;
    }

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiRect *slot = &layout->slots[index];
        const int64_t right = (int64_t)slot->x + slot->width;
        const int64_t bottom = (int64_t)slot->y + slot->height;

        if (slot->width != layout->slot_size_dip
            || slot->height != layout->slot_size_dip
            || slot->width <= 0 || slot->height <= 0
            || slot->x < layout->workspace.x
            || slot->y < layout->workspace.y
            || right > workspace_right || bottom > workspace_bottom
            || right > INT32_MAX || right < INT32_MIN
            || bottom > INT32_MAX || bottom < INT32_MIN) {
            return 0;
        }

        {
            uint32_t other;
            for (other = index + 1u; other < DECK_UI_GRID_SLOT_COUNT; ++other) {
                const DeckUiRect *candidate = &layout->slots[other];
                const int64_t candidate_right =
                    (int64_t)candidate->x + candidate->width;
                const int64_t candidate_bottom =
                    (int64_t)candidate->y + candidate->height;
                const int overlaps = (int64_t)slot->x < candidate_right
                                     && (int64_t)candidate->x < right
                                     && (int64_t)slot->y < candidate_bottom
                                     && (int64_t)candidate->y < bottom;
                if (overlaps) return 0;
            }
        }
    }

    return 1;
}

DeckUiHitTestStatus deck_ui_grid_hit_test(const DeckUiLayout *layout,
                                          int32_t x_dip,
                                          int32_t y_dip,
                                          uint32_t *slot_index) {
    uint32_t index;

    if (!layout || !slot_index) return DECK_UI_HIT_TEST_INVALID_ARGUMENT;
    if (!layout_grid_is_valid(layout)) return DECK_UI_HIT_TEST_INVALID_LAYOUT;

    for (index = 0u; index < DECK_UI_GRID_SLOT_COUNT; ++index) {
        const DeckUiRect *slot = &layout->slots[index];
        const int64_t right = (int64_t)slot->x + slot->width;
        const int64_t bottom = (int64_t)slot->y + slot->height;

        if (x_dip >= slot->x && (int64_t)x_dip < right
            && y_dip >= slot->y && (int64_t)y_dip < bottom) {
            *slot_index = index;
            return DECK_UI_HIT_TEST_HIT;
        }
    }

    return DECK_UI_HIT_TEST_OUTSIDE;
}
