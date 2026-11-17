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


static DeckUiLayoutMode layout_mode_for_shell_mode(DeckUiShellMode mode) {
    switch (mode) {
        case DECK_UI_SHELL_MODE_FULL:
            return DECK_UI_LAYOUT_MODE_WIDE;
        case DECK_UI_SHELL_MODE_INSPECTOR_COLLAPSIBLE:
            return DECK_UI_LAYOUT_MODE_MEDIUM;
        case DECK_UI_SHELL_MODE_SIDEBAR_RAIL:
            return DECK_UI_LAYOUT_MODE_NARROW;
        case DECK_UI_SHELL_MODE_COMPACT:
            return DECK_UI_LAYOUT_MODE_COMPACT;
        default:
            return DECK_UI_LAYOUT_MODE_UNSUPPORTED;
    }
}

DeckUiShellLayoutStatus deck_ui_shell_frame_compute(
    int32_t width_dip,
    int32_t height_dip,
    int compact_requested,
    DeckUiShellFrame *frame) {
    DeckUiShellFrame next;
    DeckUiShellMode resolved_mode;
    DeckUiLayoutInput layout_input;
    DeckUiLayoutStatus layout_status;
    DeckUiLayoutMode expected_layout_mode;

    if (!frame) return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    if (deck_ui_shell_mode_resolve(width_dip, compact_requested,
                                   &resolved_mode) !=
        DECK_UI_SHELL_MODE_OK) {
        return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    }

    layout_input.width_dip = width_dip;
    layout_input.height_dip = height_dip;
    layout_input.compact_mode =
        resolved_mode == DECK_UI_SHELL_MODE_COMPACT ? 1 : 0;

    layout_status = deck_ui_layout_compute(&layout_input, &next.layout);
    if (layout_status == DECK_UI_LAYOUT_TOO_SMALL) {
        return DECK_UI_SHELL_LAYOUT_TOO_SMALL;
    }
    if (layout_status != DECK_UI_LAYOUT_OK) {
        return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    }

    expected_layout_mode = layout_mode_for_shell_mode(resolved_mode);
    if (expected_layout_mode == DECK_UI_LAYOUT_MODE_UNSUPPORTED ||
        next.layout.mode != expected_layout_mode) {
        return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
    }

    next.mode = resolved_mode;
    *frame = next;
    return DECK_UI_SHELL_LAYOUT_OK;
}


static int overflow_input_is_valid(const DeckUiShellOverflowInput *input) {
    uint32_t index;

    if (!input || !input->preferred_extents_dip ||
        !bounds_are_valid(&input->bounds) ||
        (input->axis != DECK_UI_SHELL_AXIS_HORIZONTAL &&
         input->axis != DECK_UI_SHELL_AXIS_VERTICAL) ||
        input->item_count == 0u ||
        input->item_count > DECK_UI_SHELL_MAX_ITEMS ||
        input->min_extent_dip <= 0 || input->gap_dip < 0 ||
        input->overflow_extent_dip <= 0 ||
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

DeckUiShellLayoutStatus deck_ui_shell_overflow_layout_compute(
    const DeckUiShellOverflowInput *input,
    DeckUiShellOverflowLayout *layout) {
    DeckUiShellOverflowLayout next;
    DeckUiShellAxisInput all_items_input;
    DeckUiShellAxisLayout all_items_layout;
    DeckUiShellAxisInput visible_input;
    DeckUiRect visible_bounds;
    int visible_flags[DECK_UI_SHELL_MAX_ITEMS] = {0};
    int32_t visible_extents[DECK_UI_SHELL_MAX_ITEMS];
    int64_t available;
    int64_t visible_available;
    int64_t minimum_required;
    int64_t trailing_position;
    uint32_t visible_count = 0u;
    uint32_t mapped_active = DECK_UI_SHELL_NO_ACTIVE_ITEM;
    uint32_t index;
    DeckUiShellLayoutStatus status;

    if (!layout) return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    if (!overflow_input_is_valid(input)) {
        return DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT;
    }

    memset(&next, 0, sizeof(next));
    memset(&all_items_input, 0, sizeof(all_items_input));
    all_items_input.bounds = input->bounds;
    all_items_input.axis = input->axis;
    all_items_input.item_count = input->item_count;
    all_items_input.preferred_extents_dip = input->preferred_extents_dip;
    all_items_input.min_extent_dip = input->min_extent_dip;
    all_items_input.gap_dip = input->gap_dip;
    all_items_input.active_index = input->active_index;

    /*
     * Compression already handles the case where preferred sizes do not fit.
     * Overflow is necessary only when the complete list cannot fit even at
     * minimum extents; this avoids hiding items prematurely.
     */
    status = deck_ui_shell_axis_layout_compute(&all_items_input,
                                               &all_items_layout);
    if (status == DECK_UI_SHELL_LAYOUT_OK) {
        next.visible_layout = all_items_layout;
        next.visible_count = input->item_count;
        next.total_extent_dip = all_items_layout.total_extent_dip;
        for (index = 0u; index < input->item_count; ++index) {
            next.visible_indices[index] = index;
        }
        *layout = next;
        return DECK_UI_SHELL_LAYOUT_OK;
    }
    if (status != DECK_UI_SHELL_LAYOUT_TOO_SMALL) return status;

    available = input->axis == DECK_UI_SHELL_AXIS_HORIZONTAL
        ? input->bounds.width : input->bounds.height;
    visible_available = available - input->overflow_extent_dip -
                        input->gap_dip;
    if (visible_available < input->min_extent_dip ||
        visible_available > INT32_MAX) {
        return DECK_UI_SHELL_LAYOUT_TOO_SMALL;
    }

    /*
     * Reserve the active item first. If there is no active item, preserve the
     * first item. The rest are admitted in source order while their minimum
     * extents and gaps fit beside the overflow affordance.
     */
    index = input->active_index == DECK_UI_SHELL_NO_ACTIVE_ITEM
        ? 0u : input->active_index;
    visible_flags[index] = 1;
    visible_count = 1u;

    for (index = 0u; index < input->item_count; ++index) {
        uint32_t candidate_count;
        if (visible_flags[index]) continue;

        candidate_count = visible_count + 1u;
        minimum_required =
            (int64_t)candidate_count * input->min_extent_dip +
            (int64_t)(candidate_count - 1u) * input->gap_dip;
        if (minimum_required > visible_available) break;

        visible_flags[index] = 1;
        visible_count = candidate_count;
    }

    if (visible_count == 0u || visible_count >= input->item_count) {
        return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
    }

    memset(&visible_input, 0, sizeof(visible_input));
    visible_bounds = input->bounds;
    if (input->axis == DECK_UI_SHELL_AXIS_HORIZONTAL) {
        visible_bounds.width = (int32_t)visible_available;
    } else {
        visible_bounds.height = (int32_t)visible_available;
    }
    visible_input.bounds = visible_bounds;
    visible_input.axis = input->axis;
    visible_input.item_count = visible_count;
    visible_input.preferred_extents_dip = visible_extents;
    visible_input.min_extent_dip = input->min_extent_dip;
    visible_input.gap_dip = input->gap_dip;
    visible_input.active_index = DECK_UI_SHELL_NO_ACTIVE_ITEM;

    visible_count = 0u;
    for (index = 0u; index < input->item_count; ++index) {
        if (visible_flags[index]) {
            next.visible_indices[visible_count] = index;
            visible_extents[visible_count] =
                input->preferred_extents_dip[index];
            if (input->active_index == index) {
                mapped_active = visible_count;
            }
            ++visible_count;
        } else {
            next.hidden_indices[next.hidden_count++] = index;
        }
    }

    visible_input.active_index = mapped_active;
    status = deck_ui_shell_axis_layout_compute(&visible_input,
                                               &next.visible_layout);
    if (status != DECK_UI_SHELL_LAYOUT_OK) return status;

    trailing_position = input->axis == DECK_UI_SHELL_AXIS_HORIZONTAL
        ? (int64_t)input->bounds.x + input->bounds.width -
              input->overflow_extent_dip
        : (int64_t)input->bounds.y + input->bounds.height -
              input->overflow_extent_dip;
    if (trailing_position < INT32_MIN || trailing_position > INT32_MAX) {
        return DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE;
    }

    set_item_rect(&next.overflow_rect, &input->bounds, input->axis,
                  (int32_t)trailing_position,
                  input->overflow_extent_dip);
    next.visible_count = visible_count;
    next.overflow_visible = 1;
    next.total_extent_dip = (int32_t)available;
    *layout = next;
    return DECK_UI_SHELL_LAYOUT_OK;
}
