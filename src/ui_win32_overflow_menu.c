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

        if (character == L'\r' || character == L'\n' ||
            character == L'\t') {
            return 0;
        }
        if (character == L'&') {
            escaped[target++] = L'&';
        }
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
