#ifndef CDECK_HOTKEYS_H
#define CDECK_HOTKEYS_H

#define DECK_HOTKEY_MAX 8

typedef struct DeckHotkeySpec {
    unsigned int id;
    unsigned int modifiers;
    unsigned int virtual_key;
} DeckHotkeySpec;

typedef struct DeckHotkeyState {
    unsigned int registered_mask;
} DeckHotkeyState;

typedef struct DeckHotkeyReport {
    unsigned int registered_mask;
    unsigned int failed_mask;
    unsigned int unregister_failed_mask;
    unsigned long register_errors[DECK_HOTKEY_MAX];
    unsigned long unregister_errors[DECK_HOTKEY_MAX];
    int invalid_request;
} DeckHotkeyReport;

typedef int (*DeckHotkeyRegisterFn)(
    void *context,
    unsigned int id,
    unsigned int modifiers,
    unsigned int virtual_key,
    unsigned long *error_code);

typedef int (*DeckHotkeyUnregisterFn)(
    void *context,
    unsigned int id,
    unsigned long *error_code);

/*
 * Reconciles the registrations tracked by state with the requested set.
 * A failed unregister remains in registered_mask and is never double-registered.
 * Registration failures are isolated: remaining hotkeys are still attempted.
 */
DeckHotkeyReport deck_hotkeys_apply(
    DeckHotkeyState *state,
    const DeckHotkeySpec *specs,
    unsigned int count,
    int enabled,
    DeckHotkeyRegisterFn register_fn,
    DeckHotkeyUnregisterFn unregister_fn,
    void *context);

#endif
