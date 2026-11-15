#include "gdi_scope.h"
#include <string.h>

void deck_gdi_scope_init(DeckGdiScope *scope) {
    if (scope) memset(scope, 0, sizeof(*scope));
}

int deck_gdi_scope_release(HDC dc, DeckGdiScope *scope, HGDIOBJ object) {
    if (!object || object == HGDI_ERROR) return 0;

    if (dc && (GetCurrentObject(dc, OBJ_BRUSH) == object ||
               GetCurrentObject(dc, OBJ_PEN) == object)) {
        if (scope && scope->count < DECK_GDI_SCOPE_MAX) {
            scope->deferred[scope->count++] = object;
            return 1;
        }
        return 0;
    }

    return DeleteObject(object) ? 1 : 0;
}

unsigned int deck_gdi_scope_restore(HDC dc, DeckGdiScope *scope,
                                    HGDIOBJ previous_brush, HGDIOBJ previous_pen) {
    unsigned int failures = 0;
    if (!dc || !scope) return 1;

    if (previous_brush && previous_brush != HGDI_ERROR &&
        SelectObject(dc, previous_brush) == HGDI_ERROR) {
        ++failures;
    }
    if (previous_pen && previous_pen != HGDI_ERROR &&
        SelectObject(dc, previous_pen) == HGDI_ERROR) {
        ++failures;
    }

    for (unsigned int i = 0; i < scope->count; ++i) {
        if (scope->deferred[i] && scope->deferred[i] != HGDI_ERROR &&
            !DeleteObject(scope->deferred[i])) {
            ++failures;
        }
    }
    deck_gdi_scope_init(scope);
    return failures;
}
