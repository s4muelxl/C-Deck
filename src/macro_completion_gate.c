#include "macro_completion_gate.h"

void deck_macro_completion_gate_bind(DeckMacroCompletionGate *gate, HWND target) {
    if (!gate) return;
    AcquireSRWLockExclusive(&gate->lock);
    gate->target = target;
    gate->closed = target == NULL;
    ReleaseSRWLockExclusive(&gate->lock);
}

BOOL deck_macro_completion_gate_post(DeckMacroCompletionGate *gate,
                                     UINT message, WPARAM wparam, LPARAM lparam) {
    BOOL posted = FALSE;
    if (!gate) return FALSE;
    /* Serialize the closed check and PostMessageW against window destruction. */
    AcquireSRWLockShared(&gate->lock);
    if (!gate->closed && gate->target) {
        posted = PostMessageW(gate->target, message, wparam, lparam);
    }
    ReleaseSRWLockShared(&gate->lock);
    return posted;
}

void deck_macro_completion_gate_close(DeckMacroCompletionGate *gate) {
    if (!gate) return;
    AcquireSRWLockExclusive(&gate->lock);
    gate->closed = TRUE;
    gate->target = NULL;
    ReleaseSRWLockExclusive(&gate->lock);
}
