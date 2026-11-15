#ifndef CDECK_UI_LAYOUT_H
#define CDECK_UI_LAYOUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DECK_UI_GRID_COLUMNS = 5,
    DECK_UI_GRID_ROWS = 3,
    DECK_UI_GRID_SLOT_COUNT = 15
};

typedef struct DeckUiRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} DeckUiRect;

typedef enum DeckUiLayoutMode {
    DECK_UI_LAYOUT_MODE_UNSUPPORTED = 0,
    DECK_UI_LAYOUT_MODE_WIDE,
    DECK_UI_LAYOUT_MODE_MEDIUM,
    DECK_UI_LAYOUT_MODE_NARROW,
    DECK_UI_LAYOUT_MODE_COMPACT
} DeckUiLayoutMode;

typedef enum DeckUiLayoutStatus {
    DECK_UI_LAYOUT_OK = 0,
    DECK_UI_LAYOUT_TOO_SMALL,
    DECK_UI_LAYOUT_INVALID_ARGUMENT
} DeckUiLayoutStatus;

typedef struct DeckUiLayoutInput {
    /* Client-area dimensions in device-independent pixels (DIP). */
    int32_t width_dip;
    int32_t height_dip;
    /* Non-zero selects the compact layout regardless of the normal breakpoint. */
    int compact_mode;
} DeckUiLayoutInput;

typedef struct DeckUiLayout {
    DeckUiLayoutMode mode;
    DeckUiRect top_bar;
    DeckUiRect sidebar;
    DeckUiRect workspace;
    DeckUiRect inspector;
    DeckUiRect status_bar;
    DeckUiRect slots[DECK_UI_GRID_SLOT_COUNT];
    int32_t slot_size_dip;
    int32_t slot_gap_dip;
    uint32_t slot_count;
    int sidebar_visible;
    int inspector_visible;
} DeckUiLayout;

/*
 * Computes the Command Center regions and centered 5x3 grid in DIP.
 * Normal breakpoints are based on client width: wide >=1360, medium >=1120,
 * narrow >=900. Compact mode is explicit and supports the 440x320 DIP target.
 * Returns TOO_SMALL when the client area cannot preserve 44 DIP square keys.
 * No Win32 calls, allocation, persistence, or application state are involved.
 */
DeckUiLayoutStatus deck_ui_layout_compute(const DeckUiLayoutInput *input,
                                          DeckUiLayout *layout);

/* Converts DIP to physical pixels using nearest-integer rounding. dpi=0 uses 96. */
int32_t deck_ui_dip_to_px(int32_t dip, uint32_t dpi);

#ifdef __cplusplus
}
#endif

#endif
