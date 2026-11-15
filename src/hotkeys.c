#include "hotkeys.h"
#include <string.h>

DeckHotkeyReport deck_hotkeys_apply(
    DeckHotkeyState *state,
    const DeckHotkeySpec *specs,
    unsigned int count,
    int enabled,
    DeckHotkeyRegisterFn register_fn,
    DeckHotkeyUnregisterFn unregister_fn,
    void *context) {
    DeckHotkeyReport report;
    memset(&report, 0, sizeof(report));

    if (!state || !specs || count == 0 || count > DECK_HOTKEY_MAX ||
        !unregister_fn || (enabled && !register_fn)) {
        report.invalid_request = 1;
        if (state) report.registered_mask = state->registered_mask;
        return report;
    }

    for (unsigned int i = 0; i < count; ++i) {
        unsigned int bit = 1u << i;
        unsigned long error_code = 0;

        if ((state->registered_mask & bit) == 0) continue;

        if (unregister_fn(context, specs[i].id, &error_code)) {
            state->registered_mask &= ~bit;
        } else {
            report.unregister_failed_mask |= bit;
            report.unregister_errors[i] = error_code;
        }
    }

    if (enabled) {
        for (unsigned int i = 0; i < count; ++i) {
            unsigned int bit = 1u << i;
            unsigned long error_code = 0;

            /* A failed unregister means this ID is still owned by this window. */
            if (state->registered_mask & bit) continue;

            if (register_fn(context, specs[i].id, specs[i].modifiers,
                            specs[i].virtual_key, &error_code)) {
                state->registered_mask |= bit;
            } else {
                report.failed_mask |= bit;
                report.register_errors[i] = error_code;
            }
        }
    }

    report.registered_mask = state->registered_mask;
    return report;
}
