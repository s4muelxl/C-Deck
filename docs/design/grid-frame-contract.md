# C-Deck native grid frame contract

## Purpose

`ui_grid_frame.c` composes the existing C11 layout, view-state, presentation and theme contracts into one immutable snapshot that a future Win32 renderer can consume. It is the boundary between semantic UI state and drawing; it is not a renderer and does not replace the current desktop window procedure.

## Frame contents

A successful frame contains exactly 15 slots in row-major order. Each slot carries:

- Its validated square rectangle in DIP.
- The resolved semantic presentation state (normal, hover, pressed, selected, disabled, error or warning).
- The corresponding theme style: fill, text, border, indicator and renderer flags.

The caller may supply one model state per slot. Only `NORMAL`, `DISABLED`, `ERROR` and `WARNING` are accepted from the model. Pointer feedback and keyboard focus remain owned by `DeckUiViewState`. If no model-state array is supplied, every slot defaults to `NORMAL`; this is a neutral default, not a claim about runtime action availability.

## Validation and failure behavior

The builder rejects malformed layouts, slot arrays that are not row-major, invalid view-state indices, unsupported model states and null required arguments. It validates geometry through the existing half-open grid hit-test contract. It builds into a local temporary and copies to the caller only after all 15 slots succeed, so errors never expose a partially updated frame.

## Renderer responsibilities

The Win32 renderer remains responsible for:

1. Supplying model states derived from confirmed application state.
2. Converting each DIP rectangle with the rectangle-edge DPI helpers.
3. Converting semantic RGB values to `COLORREF` with `deck_ui_rgb_to_colorref`.
4. Drawing the focus ring as an independent overlay when `DECK_UI_STYLE_FOCUS_RING` is set, using `DECK_UI_COLOR_FOCUS` rather than replacing the selected/hover border.
5. Owning and releasing all GDI resources through the existing native resource-lifecycle rules.

The frame builder does not allocate GDI objects, call Win32, persist configuration, dispatch commands or execute actions. It adds no runtime dependency.

## Integration boundary

This module is intentionally independent of `src/interface.c`. The engineering PR #16 continues to own the current Win32 window procedure and action integration. Once that work is reviewed and the shared-file boundary is coordinated, a separate integration change can consume this frame in the native paint path. Until then, this contract is build/test evidence only and must not be described as an already-rendered Command Center.

## Tests

`tests/ui_grid_frame_test.c` covers all 15 slots, row-major geometry, neutral defaults, selected-plus-focused styling, disabled/error/warning precedence, malformed layout, invalid view/model states and output preservation on failure. The native UI workflow compiles and runs both the presentation resolver and grid-frame builder with strict C11 warnings on Linux and MinGW/Windows.


## Disabled-slot keyboard navigation

The grid model exposes one authoritative focusability mask through
`deck_ui_grid_focusable_mask_from_states`. It derives the mask from the same
15 model states used to build the rendered frame: only `DISABLED` slots are
excluded. `ERROR` and `WARNING` remain focusable so the user can inspect the
slot and correct its configuration.

The mask is a 15-bit value in row-major order. Directional navigation skips
disabled slots along the current row or column and remains clamped at grid
edges; it never wraps. Home/End selects the first/last focusable slot in the
current row, while Ctrl+Home/Ctrl+End-style callers can request the whole grid.
A range with no focusable slot returns an explicit status and leaves the
caller’s focus unchanged.

When a slot becomes disabled while it owns keyboard focus, the caller must
invoke `deck_ui_view_reconcile_focus` after updating the model. Reconciliation
chooses the nearest enabled slot by row-major distance, preferring the following
slot on ties, and clears stale pressed feedback. Selection remains independent
from focus, and none of these helpers invokes an action. The original
all-slots-enabled navigation functions remain available for compatibility.
