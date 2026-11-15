#include "gdi_scope.h"
#include <string.h>

void deck_gdi_scope_init(DeckGdiScope *scope) {
    if (scope) memset(scope, 0, sizeof(*scope));
}

void deck_gdi_scope_release(HDC dc, DeckGdiScope *scope, HGDIOBJ object) {
    if (!object || object == HGDI_ERROR) return;

    if (dc && (GetCurrentObject(dc, OBJ_BRUSH) == object ||
               GetCurrentObject(dc, OBJ_PEN) == object)) {
        if (scope && scope->count < DECK_GDI_SCOPE_MAX) {
            scope->deferred[scope->count++] = object;
        }
        return;
    }

    DeleteObject(object);
}

void deck_gdi_scope_restore(HDC dc, DeckGdiScope *scope,
                            HGDIOBJ previous_brush, HGDIOBJ previous_pen) {
    if (!dc || !scope) return;

    if (previous_brush && previous_brush != HGDI_ERROR) {
        SelectObject(dc, previous_brush);
    }
    if (previous_pen && previous_pen != HGDI_ERROR) {
        SelectObject(dc, previous_pen);
    }

    for (unsigned int i = 0; i < scope->count; ++i) {
        if (scope->deferred[i] && scope->deferred[i] != HGDI_ERROR) {
            DeleteObject(scope->deferred[i]);
        }
    }
    deck_gdi_scope_init(scope);
}
