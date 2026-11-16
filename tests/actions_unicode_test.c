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

    /* The largest storable UTF-8 destination is exactly 8,191 bytes.
       Use complete two-byte characters so the boundary also checks UTF-8 decoding. */
    memset(&app, 0, sizeof(app));
    app.configured = 1;
    {
        const char prefix[] = "C:\\";
        const char suffix[] = ".exe";
        size_t prefix_length = sizeof(prefix) - 1;
        size_t suffix_length = sizeof(suffix) - 1;
        size_t payload_length = DECK_PATH_TEXT_MAX - 1 - prefix_length - suffix_length;
        assert((payload_length % 2) == 0);
        memcpy(app.path, prefix, prefix_length);
        for (size_t offset = 0; offset < payload_length; offset += 2) {
            app.path[prefix_length + offset] = (char)0xC3;
            app.path[prefix_length + offset + 1] = (char)0xA9;
        }
        memcpy(app.path + prefix_length + payload_length, suffix, sizeof(suffix));
        assert(strlen(app.path) == DECK_PATH_TEXT_MAX - 1);
        assert(action_prepare_launch_target(&app, actual, DECK_PATH_WIDE_MAX) == ACTION_SUCCESS);
        assert(wcslen(actual) == prefix_length + payload_length / 2 + suffix_length);
        assert(actual[prefix_length] == (wchar_t)0x00E9);
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
