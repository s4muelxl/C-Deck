#ifndef CDECK_UI_SHELL_LAYOUT_H
#define CDECK_UI_SHELL_LAYOUT_H

#include "ui_layout.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DECK_UI_SHELL_MAX_ITEMS = 16
};

#define DECK_UI_SHELL_NO_ACTIVE_ITEM UINT32_MAX

typedef enum DeckUiShellAxis {
    DECK_UI_SHELL_AXIS_HORIZONTAL = 0,
    DECK_UI_SHELL_AXIS_VERTICAL
} DeckUiShellAxis;

typedef enum DeckUiShellLayoutStatus {
    DECK_UI_SHELL_LAYOUT_OK = 0,
    DECK_UI_SHELL_LAYOUT_TOO_SMALL,
    DECK_UI_SHELL_LAYOUT_INVALID_ARGUMENT,
    DECK_UI_SHELL_LAYOUT_OUT_OF_RANGE
} DeckUiShellLayoutStatus;

typedef struct DeckUiShellAxisInput {
    DeckUiRect bounds;
    DeckUiShellAxis axis;
    uint32_t item_count;
    const int32_t *preferred_extents_dip;
    int32_t min_extent_dip;
    int32_t gap_dip;
    /* Use DECK_UI_SHELL_NO_ACTIVE_ITEM when no item is selected. */
    uint32_t active_index;
} DeckUiShellAxisInput;

typedef struct DeckUiShellAxisLayout {
    DeckUiRect items[DECK_UI_SHELL_MAX_ITEMS];
    uint32_t item_count;
    uint32_t active_index;
    int32_t total_extent_dip;
    int compressed;
} DeckUiShellAxisLayout;

/*
 * Lays out measured navigation items on one axis without knowing their labels
 * or capabilities. This is suitable for top-bar tabs, contextual sidebar
 * rows, menu entries and icon-picker lists. The caller supplies preferred
 * extents measured with its native text/font metrics.
 *
 * Items retain their preferred sizes when they fit. If they do not, extents
 * shrink proportionally but never below min_extent_dip. Gaps are preserved.
 * The unused cross-axis dimension always fills bounds. Items are emitted in
 * input order. No wrapping, scrolling, text measurement, allocation, Win32
 * calls or application state are involved.
 *
 * The function is fail-closed: on any error, output is left unchanged. A
 * TOO_SMALL result means even minimum extents plus gaps cannot fit. Arithmetic
 * overflow returns OUT_OF_RANGE. Integer remainder pixels are assigned by
 * largest fractional remainder, with stable left-to-right/top-to-bottom ties.
 */
DeckUiShellLayoutStatus deck_ui_shell_axis_layout_compute(
    const DeckUiShellAxisInput *input,
    DeckUiShellAxisLayout *layout);

#ifdef __cplusplus
}
#endif

#endif
