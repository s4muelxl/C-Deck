# Win32 overflow menu adapter

## Purpose

The shell layout contract returns the original indices that did not fit in the
top bar or contextual sidebar. This adapter turns those indices into a native
Win32 popup menu without taking ownership of navigation state or application
commands.

The desktop remains C11/C++ with Win32. The adapter uses the Windows User32
APIs CreatePopupMenu, AppendMenuW and DestroyMenu; it introduces no web
runtime, graphics framework or third-party dependency.

## Data contract

- Pass hidden_indices from DeckUiShellOverflowLayout.hidden_indices.
- labels_by_source_index is indexed by the original source index (0–15), not
  by the position inside the hidden list. Labels must come from the real
  application model; this module does not invent category names.
- enabled_mask uses one bit per source index. Disabled entries remain visible
  but are grayed and cannot be selected.
- Each label must contain 1–128 UTF-16 code units. CR, LF and TAB are rejected.
  Ampersands are escaped so a category label cannot accidentally create a
  mnemonic.
- The caller reserves a 16-ID command range that fits in the low word of
  WM_COMMAND. Each menu item ID is command_id_base + source_index, so the
  original model index survives filtering and ordering.

The input is rejected if an index is out of range, duplicated, has no usable
label, or would produce an ambiguous command identifier. A partially built
HMENU is destroyed on platform failure. The output handle is assigned only
after every item has been appended.

## Window-procedure integration

The helper intentionally does not call TrackPopupMenu and does not dispatch
commands. The production WndProc remains the owner of the menu lifecycle:

1. Build the hidden-index list and labels from the current navigation model.
2. Call deck_ui_win32_overflow_menu_create.
3. Position the menu at the overflow rectangle converted from DIP to client
   pixels (then to screen coordinates with ClientToScreen).
4. Call TrackPopupMenu with TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON.
   The native menu supplies keyboard navigation and Escape dismissal.
5. Convert the returned command ID with
   deck_ui_win32_overflow_menu_command_to_index.
6. Verify that the resolved index is still in the current hidden_indices list,
   then emit a navigation intent. Do not execute the slot action.
7. Call DestroyMenu on every path after creation.

Rebuild the menu after a resize, DPI transition, locale/model change or enabled
state change. Do not retain an HMENU across changes to the navigation model.
Windows supplies native menu colors and high-contrast behavior.

## Validation

tests/ui_win32_overflow_menu_test.c runs on the Windows CI runner. It creates
and inspects a real popup-menu handle without displaying a window, verifies
Unicode labels, original source IDs, enabled/disabled states, reverse mapping,
invalid/duplicate indices, label bounds and 16-bit command-range validation.

This is a resource/API contract test, not a manual visual inspection of the
popup on a desktop session. The adapter is not yet connected to the production
WndProc; that integration must be coordinated with the engineering PR because
src/interface.c is owned there.
