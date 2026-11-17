#include "ui_shell_layout.h"

#include <limits.h>
#include <string.h>

static int bounds_are_valid(const DeckUiRect *bounds) {
    const int64_t right = (int64_t)bounds->x + bounds->width;
    const int64_t bottom = (int64_t)bounds->y + bounds->height;

    return bounds->width > 0 && bounds->height > 0 &&
           right >= INT32_MIN && right <= INT32_MAX &&
           bottom >= INT32_MIN && bottom <= INT32_MAX;
}

static int input_is_valid(const DeckUiShellAxisInput *input) {
    uint32_t index;

    if (!input || !input->preferred_extents_dip ||
        !bounds_are_valid(&input->bounds) ||
        (input->axis != DECK_UI_SHELL_AXIS_HORIZONTAL &&
         input->axis != DECK_UI_SHELL_AXIS_VERTICAL) ||
        input->item_count == 0u ||
        input->item_count > DECK_UI_SHELL_MAX_ITEMS ||
        input->min_extent_dip <= 0 || input->gap_dip < 0 ||
        (input->active_index != DECK_UI_SHELL_NO_ACTIVE_ITEM &&
         input->active_index >= input->item_count)) {
        return 0;
    }

    for (index = 0u; index < input->item_count; ++index) {
        if (input->preferred_extents_dip[index] < input->min_extent_dip) {
            return 0;
        }
    }
    return 1;
}

static void set_item_rect(DeckUiRect *rect,
                          const DeckUiRect *bounds,
                          DeckUiShellAxis axis,
                          int32_t position,
                          int32_t extent) {
    if (axis == DECK_UI_SHELL_AXIS_HORIZONTAL) {
        rect->x = position;
        rect->y = bounds->y;
        rect->width = extent;
        rect->height = bounds->height;
    } else {
        rect->x = bounds->x;
        rect->y = position;
        rect->width = bounds->width;
        rect->height = extent;
    }
}

DeckUiShellLayoutStatus deck_ui_shell_axis_layout_compute(
    const DeckUiShellAxisInput *input,
    DeckUiShellAxisLayout *layout) {
    DeckUiShellAxisLayout next;
    int32_t extents[DECK_UI_SHELL_MAX_ITEMS];
    int64_t remainders[DECK_UI_SHELL_MAX_ITEMS];
    int remainder_used[DECK_UI_SHELL_MAX_ITEMS];
    int64_t available;
    int64_t gap_total;
    int64_t min_total;
    int64_t preferred_total = 0;
    int64_t weight_total = 0;
    int64_t extra;
    int64_t allocated = 0;
    int64_t total_extent;
    int64_t cursor;
    uint32_t index;

    if (!layout) return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    if (!input_is_valid(input)) return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;

    available = input->axis == DECK_UI_SHELL_AXIS_HORIZONTAL
        ? input->bounds.width : input->bounds.height;
    gap_total = (int64_t)input->gap_dip *
                (int64_t)(input->item_count - 1u);
    min_total = (int64_t)input->min_extent_dip *
                (int64_t)input->item_count;
    if (gap_total > available || min_total > available - gap_total) {
        return DECK_UI_SHELL_LAYOUT_TOO_SMALL;
    }

    for (index = 0u; index < input->item_count; ++index) {
        const int32_t preferred = input->preferred_extents_dip[index];
        preferred_total += preferred;
        weight_total += (int64_t)preferred - input->min_extent_dip;
    }

    total_extent = preferred_total + gap_total;
    memset(&next, 0, sizeof(next));
    memset(remainders, 0, sizeof(remainders));
    memset(remainder_used, 0, sizeof(remainder_used));
    next.item_count = input->item_count;
    next.active_index = input->active_index;
    next.compressed = total_extent > available;

    if (!next.compressed) {
        for (index = 0u; index < input->item_count; ++index) {
            extents[index] = input->preferred_extents_dip[index];
        }
    } else {
        extra = available - gap_total - min_total;
        if (weight_total <= 0) return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;

        for (index = 0u; index < input->item_count; ++index) {
            const int64_t weight =
                (int64_t)input->preferred_extents_dip[index] -
                input->min_extent_dip;
            const int64_t product = extra * weight;
            const int64_t share = product / weight_total;

            extents[index] = input->min_extent_dip + (int32_t)share;
            remainders[index] = product % weight_total;
            allocated += share;
        }

        /* Proportional floors can leave fewer than item_count single pixels. */
        while (allocated < extra) {
            uint32_t best = DECK_UI_SHELL_MAX_ITEMS;
            int64_t best_remainder = -1;

            for (index = 0u; index < input->item_count; ++index) {
                const int32_t maximum = input->preferred_extents_dip[index];
                if (!remainder_used[index] && extents[index] < maximum &&
                    remainders[index] > best_remainder) {
                    best = index;
                    best_remainder = remainders[index];
                }
            }
            if (best == DECK_UI_SHELL_MAX_ITEMS) {
                return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
            }
            ++extents[best];
            remainder_used[best] = 1;
            ++allocated;
        }
    }

    total_extent = gap_total;
    for (index = 0u; index < input->item_count; ++index) {
        total_extent += extents[index];
    }
    if (total_extent < 0 || total_extent > available ||
        total_extent > INT32_MAX) {
        return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
    }

    cursor = input->axis == DECK_UI_SHELL_AXIS_HORIZONTAL
        ? input->bounds.x : input->bounds.y;
    for (index = 0u; index < input->item_count; ++index) {
        if (cursor < INT32_MIN || cursor > INT32_MAX ||
            extents[index] <= 0) {
            return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
        }
        set_item_rect(&next.items[index], &input->bounds, input->axis,
                      (int32_t)cursor, extents[index]);
        cursor += extents[index];
        if (index + 1u < input->item_count) cursor += input->gap_dip;
    }

    next.total_extent_dip = (int32_t)total_extent;
    *layout = next;
    return DECK_UI_SHELL_LAYOUT_OK;
}

DeckUiShellModeStatus deck_ui_shell_mode_resolve(
    int32_t available_width_dip,
    int compact_requested,
    DeckUiShellMode *mode) {
    DeckUiShellMode resolved;

    if (!mode || available_width_dip <= 0 ||
        (compact_requested != 0 && compact_requested != 1)) {
        return DECK_UI_SHELL_MODE_INVALID_ARGUMENT;
    }

    if (compact_requested ||
        available_width_dip < DECK_UI_SHELL_MODE_SIDEBAR_RAIL_MIN_WIDTH_DIP) {
        resolved = DECK_UI_SHELL_MODE_COMPACT;
    } else if (available_width_dip <
               DECK_UI_SHELL_MODE_INSPECTOR_MIN_WIDTH_DIP) {
        resolved = DECK_UI_SHELL_MODE_SIDEBAR_RAIL;
    } else if (available_width_dip <
               DECK_UI_SHELL_MODE_FULL_MIN_WIDTH_DIP) {
        resolved = DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE;
    } else {
        resolved = DECK_UI_SHELL_MODE_FULL;
    }

    *mode = resolved;
    return DECK_UI_SHELL_MODE_OK;
}
