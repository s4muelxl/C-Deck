# Native shell axis layout contract

## Status

This is a C11 geometry contract for native Win32 presentation. It is not a
window, a renderer, or an application navigation implementation. It does not
define which tabs, categories, profiles, devices, or actions exist.

## Purpose

The Command Center needs consistent geometry for horizontal top-bar navigation
and vertical contextual lists. The shell-axis layout function lays out
caller-supplied items on one axis, so the same tested rule can be reused by the
top bar, sidebar, menus, and icon-picker lists without coupling those surfaces
to application state.

The caller supplies preferred extents measured using the active native font
and localized label. With GDI, measure text using the same selected HFONT and
GetTextExtentPoint32W, then add the component's horizontal or vertical padding.
The contract does not guess label widths or hard-code feature names.

## Sizing rules

- All inputs and outputs are in DIP.
- Items retain preferred extents when the list fits.
- If the preferred total does not fit, the available extent is distributed
  proportionally above each item's minimum.
- Integer remainder units use largest-remainder allocation with stable
  left-to-right or top-to-bottom tie-breaking.
- Gaps never shrink.
- The cross-axis extent fills the supplied bounds.
- Item order is preserved; active_index is metadata only and does not change
  geometry or activate a command.
- A list that cannot fit its minimum extents returns TOO_SMALL without
  modifying the output. The caller must choose an explicit compact/rail,
  scrolling, overflow-menu, or clipping policy; this helper never silently
  wraps or truncates labels.

The helper is capped at 16 items to keep the result fixed-size and allocation
free. That covers a small top navigation set and typical contextual lists while
keeping the contract bounded.

## DPI and rendering

Compute the shell layout in DIP after the client bounds are known. Convert each
result rectangle with deck_ui_rect_dip_to_px, which scales all four edges so
adjacent regions stay aligned at fractional DPI values. Recompute after client
resize or a DPI transition. Text measurement and ellipsis remain renderer
responsibilities.

## Safety and validation

The function rejects invalid axes, item counts, active indices, non-positive
bounds/minimums, negative gaps, and preferred extents below the minimum.
Bounds and aggregate arithmetic are checked before producing rectangles.
Output is assigned only after the complete layout succeeds.

The regression suite covers preferred-size horizontal tabs, proportional
compression, stable remainder allocation, vertical sidebar rows, negative
virtual-screen origins, too-small output preservation, invalid input
preservation, maximum item count, and near-INT32_MAX arithmetic.

## Integration boundary

This module is independent of src/interface.c and the action engine. The
engineering PR owns the production WndProc and current sidebar/category
behavior. Once that integration point is coordinated, the WndProc can measure
its actual localized labels, call this helper for the top bar/sidebar, and
paint the returned rectangles with the existing GDI renderer.

No web runtime, Direct2D migration, heap allocation, or additional dependency
is introduced.

## Responsive shell composition policy

`deck_ui_shell_mode_resolve` maps the available client width (in DIP) to a
presentation composition using the initial breakpoints from
`command-center-native.md`:

- **1360 DIP and above — FULL:** complete top navigation, sidebar and inspector.
- **1120–1359 DIP — INSPECTOR_COLLAPSIBLE:** full sidebar; inspector may be
  collapsed to preserve workspace width.
- **900–1119 DIP — SIDEBAR_RAIL:** sidebar becomes an icon rail and the
  inspector is treated as a temporary panel.
- **Below 900 DIP — COMPACT:** secondary navigation and inspector are hidden;
  the grid/profile remain the primary workspace.

An explicit compact preference overrides the width-derived mode. Widths must be
positive; the preference must be exactly 0 or 1. Invalid arguments return
`DECK_UI_SHELL_MODE_INVALID_ARGUMENT` without modifying the output.

This resolver chooses composition only. It does not measure text, position
controls, guarantee that a 5×3 grid fits, or change application state. The
caller must still run the grid and axis layout contracts and enforce the
window's supported minimum size. These thresholds are initial design values;
they require a Windows visual smoke test at 100%, 125%, 150% and 200% DPI
before being treated as final product behavior.

Regression tests cover both sides of every threshold, the explicit compact
override, invalid widths/preferences and output preservation.


## Coordinated shell frame

Use `deck_ui_shell_frame_compute` as the single entry point when the caller
needs both a responsive composition and its corresponding 5x3 geometry. It
runs the shell-mode resolver, translates the result into the existing layout
engine's compact flag, computes the top bar/sidebar/workspace/inspector/status
bar and grid, then verifies that both mode enums still agree.

This resolves the below-900-DIP boundary explicitly: the shell resolver selects
COMPACT and the coordinated function passes `compact_mode=1` to the grid
layout engine. An explicit compact preference has the same effect at any width.
If the chosen composition cannot preserve the 44-DIP key target or available
height, the function returns TOO_SMALL and leaves the caller's frame unchanged.

The lower-level `deck_ui_layout_compute` remains available for callers that
already selected a composition. Its `compact_mode` field is now a strict
boolean: 0 means normal width-based layout, 1 means compact, and every other
value is INVALID_ARGUMENT. This prevents negative or corrupted preference
values from silently forcing compact mode.

Regression tests exercise both sides of all shell breakpoints, verify the
shell/layout mode mapping and 15-slot validation, test explicit compact
override, and verify unchanged output for too-small or invalid requests.


## Overflow-aware navigation

`deck_ui_shell_overflow_layout_compute` adds a deterministic overflow
policy for top-bar tabs and contextual sidebar lists when the complete list
cannot fit even at its minimum item extents.

- The active item is always retained. If there is no active item, the first
  source item is retained.
- Remaining items are admitted in source order while their minimum extents and
  gaps fit beside the overflow affordance.
- Visible items are passed through the existing proportional compression
  algorithm; hidden source indices are returned in their original order.
- The overflow affordance is pinned to the trailing edge (right for horizontal
  navigation, bottom for vertical navigation).
- If the complete list can fit at minimum extents, the existing compression
  layout is used and no overflow affordance is reserved.
- A single-item list never creates an overflow affordance.

The result is geometry only. The caller remains responsible for drawing the
overflow control, opening a menu containing the returned hidden indices,
preserving semantic labels and wiring keyboard access. The helper does not
change application selection or execute commands. It is fixed-size, capped at
16 items, allocation-free and transactional on errors.

Regression coverage includes active-item retention, source-order admission,
horizontal and vertical trailing placement, compression before hiding,
single-item behavior, too-small bounds and invalid-input output preservation.

The overflow affordance has its own explicit extent and may be narrower than a text-bearing navigation item (for example, a 44-DIP icon target beside tabs with a 60-DIP minimum). It must still have a positive extent; the caller is responsible for applying the platform's target-size and accessibility requirements.
