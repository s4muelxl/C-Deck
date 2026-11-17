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
