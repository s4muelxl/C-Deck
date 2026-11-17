#ifndef CDECK_UI_WIN32_OVERFLOW_MENU_H
#define CDECK_UI_WIN32_OVERFLOW_MENU_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "ui_shell_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS = 128,
    DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_ID_MAX = 0xFFFF
};

typedef enum DeckUiWin32OverflowMenuStatus {
    DECK_UI_WIN32_OVERFLOW_MENU_OK = 0,
    DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT,
    DECK_UI_WIN32_OVERFLOW_MENU_PLATFORM_ERROR,
    DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_NOT_FOUND,
    DECK_UI_WIN32_OVERFLOW_MENU_STALE_COMMAND,
    DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_DISABLED
} DeckUiWin32OverflowMenuStatus;

typedef struct DeckUiWin32OverflowMenuInput {
    /* Source indices returned by DeckUiShellOverflowLayout.hidden_indices. */
    const uint32_t *hidden_indices;
    uint32_t hidden_count;

    /*
     * Labels indexed by original navigation source index (0..15), not by
     * hidden-list position. The menu copies each label during creation.
     */
    const wchar_t *const *labels_by_source_index;

    /* Bit N enables source item N; unset bits create disabled menu entries. */
    uint16_t enabled_mask;

    /*
     * Reserve [command_id_base, command_id_base + 15] for this menu.
     * IDs must fit LOWORD(WM_COMMAND), so the complete range must fit 16 bits.
     */
    UINT command_id_base;
} DeckUiWin32OverflowMenuInput;

/*
 * Creates a native popup menu for the hidden navigation items.
 *
 * The returned HMENU is owned by the caller and must be released with
 * DestroyMenu after TrackPopupMenu/TrackPopupMenuEx returns. The function
 * never displays the menu and never changes application selection.
 *
 * Labels are copied, must be non-empty and at most 128 UTF-16 code units.
 * Ampersands are escaped so user/category text cannot accidentally define
 * menu mnemonics. CR, LF and TAB are rejected to keep each label a single
 * plain menu row.
 *
 * Output is assigned only after every menu item has been appended. On error,
 * the output handle is left unchanged and any partially built menu is freed.
 */
DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_create(
    const DeckUiWin32OverflowMenuInput *input,
    HMENU *menu_out);

/*
 * Maps a TPM_RETURNCMD/LOWORD(WM_COMMAND) identifier back to its original
 * source index. The caller must also verify that the returned index belongs
 * to the current hidden_indices list before dispatching a navigation intent.
 */
DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_command_to_index(
    UINT command_id,
    UINT command_id_base,
    uint32_t *source_index);

/*
 * Resolves a menu command only if its source index is still present in the
 * current hidden-index list. This rejects commands returned by a stale menu
 * after a resize, locale change or navigation-model update. It also checks
 * the current enabled mask so an item disabled while the popup was open cannot
 * be activated. A zero count with a NULL list makes every command stale.
 * Output is transactional.
 */
DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_resolve_hidden_command(
    UINT command_id,
    UINT command_id_base,
    const uint32_t *current_hidden_indices,
    uint32_t current_hidden_count,
    uint16_t current_enabled_mask,
    uint32_t *source_index);

#ifdef __cplusplus
}
#endif

#endif
