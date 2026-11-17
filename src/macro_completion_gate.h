#ifndef CDECK_MACRO_COMPLETION_GATE_H
#define CDECK_MACRO_COMPLETION_GATE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct DeckMacroCompletionGate {
    SRWLOCK lock;
    HWND target;
    BOOL closed;
} DeckMacroCompletionGate;

/* Static initializer keeps the SRWLOCK valid across bind/close cycles. */
#define DECK_MACRO_COMPLETION_GATE_INITIALIZER { SRWLOCK_INIT, NULL, TRUE }

void deck_macro_completion_gate_bind(DeckMacroCompletionGate *gate, HWND target);
BOOL deck_macro_completion_gate_post(DeckMacroCompletionGate *gate,
                                     UINT message, WPARAM wparam, LPARAM lparam);
void deck_macro_completion_gate_close(DeckMacroCompletionGate *gate);

#endif
