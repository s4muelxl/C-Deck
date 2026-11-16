# Isolated Win32 grid renderer

## Status

This is an isolated native rendering component and an off-screen Win32 proof. It is **not yet connected to the production window procedure**. The application remains C11/C++ with Win32; no web runtime or graphics framework is introduced.

## Responsibilities

The renderer consumes a validated DeckUiGridFrame and paints the 15 slot surfaces into a caller-owned HDC.

- Converts each slot rectangle from DIP to physical pixels by scaling its edges, avoiding fractional-DPI seams.
- Uses the existing semantic palette for fill, text, border, indicator and focus.
- Draws an independent inner focus ring, preserving a selected slot's lime border and fill.
- Accepts optional borrowed HICON and UTF-16 label arrays. It never destroys caller-owned icons or strings.
- Uses Segoe UI through GDI; no external font package or graphics dependency is required.

## Slot content layout and defensive frame validation

The renderer validates the complete 5×3 row-major geometry again at its boundary:
all slots must be square and equal-sized, horizontal/vertical gaps must match,
and the gap must match the selected layout mode (14 DIP for normal modes,
8 DIP for compact). Each slot's semantic style is also checked against the
renderer palette and its resolved visual/focus state. A malformed or manually
modified frame is rejected before the target HDC is touched.

The function deck_ui_win32_grid_renderer_content_layout computes icon and label
rectangles for each physical-pixel slot. When both are present, the icon is
reduced to fit above the label with a DPI-scaled gap; it no longer overlaps the
label at the 44–49 DIP compact key sizes. Icon-only slots keep a centered icon,
and label-only slots retain the lower caption area. The helper leaves its output
unchanged for invalid arguments or invalid geometry.

The Windows regression suite checks the content rectangles at 96 DPI and 48
DPI, paints a 45 DIP compact grid at 150% scaling with both icon and label,
and rejects overlapping geometry, a style inconsistent with focus state, and
a frame whose layout mode disagrees with its slot gap.

## Resource and paint lifecycle

The renderer creates brushes, pens and one label font during initialization and reuses them for every paint. It creates no GDI objects inside the 15-slot loop.

Every paint is enclosed by SaveDC / RestoreDC, so cached objects are not left selected in the target HDC. Destroy reports DeleteObject failures and retains handles that could not be released, allowing a caller to retry after resolving an external selection.

The renderer is UI-thread confined. Callers must not destroy or mutate it concurrently with a paint. The target HDC and optional icon handles remain caller-owned.

## Automated Windows evidence

The Windows test paints to a compatible memory DC at 144 DPI and checks:

- Normal, selected and disabled fill pixels.
- The selected lime indicator and independent keyboard-focus ring.
- Unicode label drawing and a borrowed stock icon.
- HDC brush, pen and font restoration after paint.
- Stable GDI object count across 64 consecutive paints.
- Complete release of renderer-owned GDI objects.
- Rejection of an incomplete frame before drawing.

The native UI workflow compiles this test with strict C11 warnings and links only gdi32 and user32.

## Integration boundary

The engineering PR #16 owns src/interface.c, the production window procedure and the existing GDI resource scope. This PR adds only new renderer files, its isolated test, documentation and its own native-UI workflow. No shared application file is modified.

After the engineering branch is reviewed and the shared integration boundary is coordinated, a follow-up can call this renderer from WM_PAINT and feed it labels/icons from confirmed application state. Until then, the off-screen test proves GDI drawing/resource behavior, not the appearance of the live application window.

No Direct2D/DirectWrite migration is proposed: GDI is sufficient for this bounded proof, and the renderer adds no runtime dependency.
