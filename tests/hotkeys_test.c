#include "../src/hotkeys.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct MockHotkeys {
    unsigned int registered_mask;
    unsigned int fail_register_mask;
    unsigned int fail_unregister_mask;
    unsigned int register_calls[3];
    unsigned int unregister_calls[3];
} MockHotkeys;

static int mock_register(void *context, unsigned int id, unsigned int modifiers,
                         unsigned int virtual_key, unsigned long *error_code) {
    MockHotkeys *mock = (MockHotkeys *)context;
    unsigned int index;
    unsigned int bit;

    assert(mock != NULL);
    assert(id >= 1 && id <= 3);
    index = id - 1;
    bit = 1u << index;
    ++mock->register_calls[index];
    assert(modifiers == 0x0003u);

    if (index == 0) assert(virtual_key == 0x26u);
    if (index == 1) assert(virtual_key == 0x28u);
    if (index == 2) assert(virtual_key == 0x4du);

    if (mock->fail_register_mask & bit) {
        if (error_code) *error_code = 1409ul;
        return 0;
    }

    mock->registered_mask |= bit;
    if (error_code) *error_code = 0;
    return 1;
}

static int mock_unregister(void *context, unsigned int id, unsigned long *error_code) {
    MockHotkeys *mock = (MockHotkeys *)context;
    unsigned int index;
    unsigned int bit;

    assert(mock != NULL);
    assert(id >= 1 && id <= 3);
    index = id - 1;
    bit = 1u << index;
    ++mock->unregister_calls[index];

    if (mock->fail_unregister_mask & bit) {
        if (error_code) *error_code = 5ul;
        return 0;
    }

    mock->registered_mask &= ~bit;
    if (error_code) *error_code = 0;
    return 1;
}

int main(void) {
    const DeckHotkeySpec specs[3] = {
        { 1u, 0x0003u, 0x26u },
        { 2u, 0x0003u, 0x28u },
        { 3u, 0x0003u, 0x4du }
    };
    DeckHotkeyState state;
    DeckHotkeyReport report;
    MockHotkeys mock;

    memset(&state, 0, sizeof(state));
    memset(&mock, 0, sizeof(mock));

    report = deck_hotkeys_apply(&state, specs, 3, 1, mock_register, mock_unregister, &mock);
    assert(!report.invalid_request);
    assert(report.registered_mask == 0x7u);
    assert(state.registered_mask == 0x7u);
    assert(mock.registered_mask == 0x7u);

    /* One global conflict must not prevent the other shortcuts from registering. */
    mock.fail_register_mask = 0x2u;
    report = deck_hotkeys_apply(&state, specs, 3, 1, mock_register, mock_unregister, &mock);
    assert(report.failed_mask == 0x2u);
    assert(report.register_errors[1] == 1409ul);
    assert(report.registered_mask == 0x5u);
    assert(mock.registered_mask == 0x5u);

    /* If Windows refuses to unregister an existing ID, do not register it twice. */
    mock.fail_register_mask = 0;
    mock.fail_unregister_mask = 0x1u;
    {
        unsigned int calls_before = mock.register_calls[0];
        report = deck_hotkeys_apply(&state, specs, 3, 1, mock_register, mock_unregister, &mock);
        assert(report.unregister_failed_mask == 0x1u);
        assert(report.unregister_errors[0] == 5ul);
        assert(mock.register_calls[0] == calls_before);
        assert(state.registered_mask == 0x7u);
        assert(mock.registered_mask == 0x7u);
    }

    /* Disabling reports any registration Windows could not release. */
    report = deck_hotkeys_apply(&state, specs, 3, 0, mock_register, mock_unregister, &mock);
    assert(report.unregister_failed_mask == 0x1u);
    assert(state.registered_mask == 0x1u);
    assert(mock.registered_mask == 0x1u);

    mock.fail_unregister_mask = 0;
    report = deck_hotkeys_apply(&state, specs, 3, 0, mock_register, mock_unregister, &mock);
    assert(!report.invalid_request);
    assert(report.registered_mask == 0);
    assert(state.registered_mask == 0);
    assert(mock.registered_mask == 0);

    report = deck_hotkeys_apply(&state, specs, DECK_HOTKEY_MAX + 1u, 1,
                                mock_register, mock_unregister, &mock);
    assert(report.invalid_request);

    puts("hotkey registration, conflicts and cleanup: OK");
    return 0;
}
