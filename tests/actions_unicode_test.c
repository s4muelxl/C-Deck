#include "actions.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void assert_converts(const char *utf8, const wchar_t *expected) {
    DeckApp app;
    wchar_t actual[DECK_PATH_WIDE_MAX];
    memset(&app, 0, sizeof(app));
    app.configured = 1;
    assert(strlen(utf8) < sizeof(app.path));
    strcpy(app.path, utf8);
    assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_SUCCESS);
    assert(wcscmp(actual, expected) == 0);
}

int main(void) {
    DeckApp app;
    wchar_t actual[DECK_PATH_WIDE_MAX];
    const char invalid_utf8[] = "C:\\Apps\\bad\xC3\x28.exe";

    /* The production launcher uses this exact conversion before ShellExecuteW. */
    assert_converts("C:\\Apps\\音楽播放器\\Café.exe",
                    L"C:\\Apps\\音楽播放器\\Café.exe");
    assert_converts("https://例え.テスト/路径?q=ação#seção",
                    L"https://例え.テスト/路径?q=ação#seção");
    assert_converts("https://example.com/ação%20ao%20vivo",
                    L"https://example.com/ação%20ao%20vivo");

    /* Destination fields now accept paths well beyond the former 260-byte cap. */
    memset(&app, 0, sizeof(app));
    app.configured = 1;
    strcpy(app.path, "C:\\Apps\\");
    {
        size_t prefix_length = strlen(app.path);
        memset(app.path + prefix_length, 'A', 1024);
        app.path[prefix_length + 1024] = '\0';
        assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_SUCCESS);
        assert(wcslen(actual) == prefix_length + 1024);
        assert(actual[prefix_length] == L'A');
    }

    memset(&app, 0, sizeof(app));
    app.configured = 1;
    strcpy(app.path, invalid_utf8);
    memset(actual, 0x7f, sizeof(actual));
    assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_LAUNCH_FAILED);
    assert(actual[0] == L'\0');

    strcpy(app.path, "C:\\Apps\\long-name.exe");
    actual[0] = L'X';
    assert(action_prepare_launch_target(&app, actual, 4) == ACTION_LAUNCH_FAILED);
    assert(actual[0] == L'\0');

    memset(&app, 0, sizeof(app));
    app.configured = 0;
    actual[0] = L'X';
    assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_NOT_CONFIGURED);
    assert(actual[0] == L'\0');

    memset(&app, 0, sizeof(app));
    app.configured = 1;
    memset(app.path, 'A', sizeof(app.path)); /* no in-bounds NUL terminator */
    actual[0] = L'X';
    assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_LAUNCH_FAILED);
    assert(actual[0] == L'\0');

    assert(action_prepare_launch_target(NULL, actual, DECK_PATH_WIDE_MAX) == ACTION_INVALID_ARGUMENT);
    assert(action_prepare_launch_target(&app, NULL, DECK_TEXT_MAX) == ACTION_INVALID_ARGUMENT);
    assert(action_prepare_launch_target(&app, actual, 0) == ACTION_INVALID_ARGUMENT);

    puts("Unicode launch target conversion: OK");
    return 0;
}
