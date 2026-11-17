#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui_win32_overflow_menu.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void read_menu_item(HMENU menu,
                           UINT position,
                           MENUITEMINFOW *item,
                           wchar_t text[160]) {
    memset(item, 0, sizeof(*item));
    memset(text, 0, sizeof(wchar_t) * 160u);
    item->cbSize = sizeof(*item);
    item->fMask = MIIM_ID | MIIM_STATE | MIIM_STRING;
    item->dwTypeData = text;
    item->cch = 159u;
    assert(GetMenuItemInfoW(menu, position, TRUE, item));
}

static void test_native_menu_preserves_source_ids_and_unicode_labels(void) {
    const uint32_t hidden[] = { 4u, 1u, 7u };
    const wchar_t *labels[DECK_UI_SHELL_MAX_ITEMS] = { 0 };
    const UINT command_base = 0x5100u;
    DeckUiWin32OverflowMenuInput input;
    HMENU menu = NULL;
    MENUITEMINFOW item;
    wchar_t text[160];
    uint32_t source_index = 99u;

    labels[4] = L"Áudio";
    labels[1] = L"Aplicativos";
    labels[7] = L"Navegação";

    input.hidden_indices = hidden;
    input.hidden_count = 3u;
    input.labels_by_source_index = labels;
    input.enabled_mask = (uint16_t)((1u << 4u) | (1u << 7u));
    input.command_id_base = command_base;

    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_OK);
    assert(menu != NULL);
    assert(GetMenuItemCount(menu) == 3);

    read_menu_item(menu, 0u, &item, text);
    assert(item.wID == command_base + 4u);
    assert(wcscmp(text, L"Áudio") == 0);
    assert((item.fState & (MFS_DISABLED | MFS_GRAYED)) == 0u);

    read_menu_item(menu, 1u, &item, text);
    assert(item.wID == command_base + 1u);
    assert(wcscmp(text, L"Aplicativos") == 0);
    assert((item.fState & (MFS_DISABLED | MFS_GRAYED)) != 0u);

    read_menu_item(menu, 2u, &item, text);
    assert(item.wID == command_base + 7u);
    assert(wcscmp(text, L"Navegação") == 0);
    assert((item.fState & (MFS_DISABLED | MFS_GRAYED)) == 0u);

    assert(deck_ui_win32_overflow_menu_command_to_index(
               command_base + 7u, command_base, &source_index) ==
           DECK_UI_WIN32_OVERFLOW_MENU_OK);
    assert(source_index == 7u);

    source_index = 99u;
    assert(deck_ui_win32_overflow_menu_command_to_index(
               command_base + 20u, command_base, &source_index) ==
           DECK_UI_WIN32_OVERFLOW_MENU_COMMAND_NOT_FOUND);
    assert(source_index == 99u);

    assert(DestroyMenu(menu));
}

static void test_invalid_input_preserves_menu_output(void) {
    const uint32_t duplicate_indices[] = { 2u, 2u };
    const uint32_t out_of_range_indices[] = { DECK_UI_SHELL_MAX_ITEMS };
    const wchar_t *labels[DECK_UI_SHELL_MAX_ITEMS] = { 0 };
    DeckUiWin32OverflowMenuInput input;
    HMENU menu = (HMENU)(UINT_PTR)0x1234u;

    labels[2] = L"Áudio";
    input.hidden_indices = duplicate_indices;
    input.hidden_count = 2u;
    input.labels_by_source_index = labels;
    input.enabled_mask = 0xFFFFu;
    input.command_id_base = 0x5200u;

    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == (HMENU)(UINT_PTR)0x1234u);

    input.hidden_indices = out_of_range_indices;
    input.hidden_count = 1u;
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == (HMENU)(UINT_PTR)0x1234u);
}

static void test_labels_and_command_ranges_are_validated(void) {
    const uint32_t hidden[] = { 3u };
    const wchar_t *labels[DECK_UI_SHELL_MAX_ITEMS] = { 0 };
    wchar_t long_label[
        DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS + 2];
    DeckUiWin32OverflowMenuInput input;
    HMENU menu = NULL;
    uint32_t source_index = 55u;

    {
        size_t index;
        for (index = 0u;
             index < DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS + 1u;
             ++index) {
            long_label[index] = L'x';
        }
        long_label[DECK_UI_WIN32_OVERFLOW_MENU_LABEL_MAX_CHARS + 1u] = L'\0';
    }

    input.hidden_indices = hidden;
    input.hidden_count = 1u;
    input.labels_by_source_index = labels;
    input.enabled_mask = 0xFFFFu;
    input.command_id_base = 0x5300u;

    labels[3] = L"";
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == NULL);

    labels[3] = long_label;
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == NULL);

    labels[3] = L"Áudio\tDetalhes";
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == NULL);

    labels[3] = L"Áudio";
    input.command_id_base = 0xFFF8u;
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == NULL);

    assert(deck_ui_win32_overflow_menu_command_to_index(
               0x5303u, 0xFFF8u, &source_index) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(source_index == 55u);

    input.command_id_base = UINT_MAX;
    assert(deck_ui_win32_overflow_menu_create(&input, &menu) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(menu == NULL);
    assert(deck_ui_win32_overflow_menu_command_to_index(
               UINT_MAX, UINT_MAX, &source_index) ==
           DECK_UI_WIN32_OVERFLOW_MENU_INVALID_ARGUMENT);
    assert(source_index == 55u);
}

int main(void) {
    test_native_menu_preserves_source_ids_and_unicode_labels();
    test_invalid_input_preserves_menu_output();
    test_labels_and_command_ranges_are_validated();
    puts("ui_win32_overflow_menu_test: all tests passed");
    return 0;
}
