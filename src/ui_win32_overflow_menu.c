#include "ui_win32_overflow_menu.h"

#include <wchar.h>

static int command_range_is_valid(UINT command_id_base) {
    const uint64_t last_id =
        (uint64_t)command_id_base + DECK_UI_SHELL_MAX_ITEMS - 1u;

    return command_id_base != 0u &&
           last_id <= DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_ID_MAX;
}

static int escape_menu_label(const wchar_t *label,
                             wchar_t escaped[
                                 DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS * 2 + 1]) {
    size_t length;
    size_t source;
    size_t target = 0u;

    if (!label) return 0;
    length = wcslen(label);
    if (length == 0u ||
        length > DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS) {
        return 0;
    }

    for (source = 0u; source < length; ++source) {
        const wchar_t character = label[source];

        const uint32_t code_unit =
            (uint32_t)(unsigned short)character;

        /* Win32 wchar_t is a UTF-16 code unit; reject controls and bad pairs. */
        if (code_unit < 0x20u || code_unit == 0x7Fu) return 0;
        if (code_unit >= 0xD800u && code_unit <= 0xDBFFu) {
            uint32_t low_surrogate;

            if (source + 1u >= length) return 0;
            low_surrogate =
                (uint32_t)(unsigned short)label[source + 1u];
            if (low_surrogate < 0xDC00u || low_surrogate > 0xDFFFu) {
                return 0;
            }
            escaped[target++] = character;
            escaped[target++] = label[++source];
            continue;
        }
        if (code_unit >= 0xDC00u && code_unit <= 0xDFFFu) return 0;

        if (character == L'&') escaped[target++] = L'&';
        escaped[target++] = character;
    }

    escaped[target] = L'\0';
    return 1;
}

static int input_is_valid(const DeckUiWin32OverflowMenuInput *input) {
    uint16_t seen = 0u;
    uint32_t position;

    if (!input || !input->hidden_indices ||
        !input->labels_by_source_index ||
        input->hidden_count == 0u ||
        input->hidden_count > DECK_UI_SHELL_MAX_ITEMS ||
        !command_range_is_valid(input->command_id_base)) {
        return 0;
    }

    for (position = 0u; position < input->hidden_count; ++position) {
        const uint32_t index = input->hidden_indices[position];
        uint16_t bit;
        wchar_t escaped[
            DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS * 2 + 1];

        if (index >= DECK_UI_SHELL_MAX_ITEMS) return 0;
        bit = (uint16_t)(1u << index);
        if ((seen & bit) != 0u ||
            !escape_menu_label(input->labels_by_source_index[index],
                               escaped)) {
            return 0;
        }
        seen = (uint16_t)(seen | bit);
    }

    return 1;
}

DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_create(
    const DeckUiWin32OverflowMenuInput *input,
    HMENU *menu_out) {
    HMENU menu;
    uint32_t position;

    if (!menu_out || !input_is_valid(input)) {
        return DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT;
    }

    menu = CreatePopupMenu();
    if (!menu) return DECK_UI_WIN32_OVERFLOW_MENU_PLATFORM_ERROR;

    for (position = 0u; position < input->hidden_count; ++position) {
        const uint32_t source_index = input->hidden_indices[position];
        const UINT command_id =
            input->command_id_base + (UINT)source_index;
        const UINT flags = MF_STRING |
            ((input->enabled_mask & (uint16_t)(1u << source_index)) != 0u
                ? 0u : MF_GRAYED);
        wchar_t escaped[
            DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS * 2 + 1];

        if (!escape_menu_label(
                input->labels_by_source_index[source_index], escaped) ||
            !AppendMenuW(menu, flags, (UINT_PTR)command_id, escaped)) {
            (void)DestroyMenu(menu);
            return DECK_UI_WIN32_OVERFLOW_MENU_PLATFORM_ERROR;
        }
    }

    *menu_out = menu;
    return DECK_UI_WIN32_OVERFLOW_MENU_OK;
}

DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_command_to_index(
    UINT command_id,
    UINT command_id_base,
    uint32_t *source_index) {
    uint32_t resolved;

    if (!source_index || !command_range_is_valid(command_id_base)) {
        return DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT;
    }
    if (command_id < command_id_base ||
        (uint32_t)command_id >=
            (uint32_t)command_id_base + DECK_UI_SHELL_MAX_ITEMS) {
        return DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_NOT_FOUND;
    }

    resolved = (uint32_t)command_id - command_id_base;
    *source_index = resolved;
    return DECK_UI_WIN32_OVERFLOW_MENU_OK;
}

DeckUiWin32OverflowMenuStatus deck_ui_win32_overflow_menu_resolve_hidden_command(
    UINT command_id,
    UINT command_id_base,
    const uint32_t *current_hidden_indices,
    uint32_t current_hidden_count,
    uint16_t current_enabled_mask,
    uint32_t *source_index) {
    uint32_t resolved;
    uint32_t position;
    uint16_t seen = 0u;
    int is_still_hidden = 0;
    DeckUiWin32OverflowMenuStatus status;

    if (!source_index || current_hidden_count > DECK_UI_SHELL_MAX_ITEMS ||
        (current_hidden_count > 0u && !current_hidden_indices) ||
        !command_range_is_valid(command_id_base)) {
        return DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT;
    }

    status = deck_ui_win32_overflow_menu_command_to_index(
        command_id, command_id_base, &resolved);
    if (status != DECK_UI_WIN32_OVERFLOW_MENU_OK) return status;

    for (position = 0u; position < current_hidden_count; ++position) {
        const uint32_t index = current_hidden_indices[position];
        uint16_t bit;

        if (index >= DECK_UI_SHELL_MAX_ITEMS) {
            return DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT;
        }
        bit = (uint16_t)(1u << index);
        if ((seen & bit) != 0u) {
            return DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT;
        }
        seen = (uint16_t)(seen | bit);
        if (index == resolved) is_still_hidden = 1;
    }

    if (!is_still_hidden) return DECK_UI_WIN32_OVERFLOW_MENU_STALE_COMMAND;
    if ((current_enabled_mask & (uint16_t)(1u << resolved)) == 0u) {
        return DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_DISABLED;
    }

    *source_index = resolved;
    return DECK_UI_WIN32_OVERFLOW_MENU_OK;
}
