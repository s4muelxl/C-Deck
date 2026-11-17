#include "gdi_scope.h"
#include <string.h>

void deck_gdi_scope_init(DeckGdiScope *scope) {
    if (scope) memset(scope, 0, sizeof(*scope));
}

int deck_gdi_scope_release(HDC dc, DeckGdiScope *scope, HGDIOBJ object) {
    if (!object || object == HGDI_ERROR) return 0;

    /* A queued handle remains owned by this scope even if the caller has
       already changed the HDC selection. Never delete it a second time. */
    if (scope) {
        if (scope->count > DECK_GDI_SCOPE_MAX) return 0;
        for (unsigned int i = 0; i < scope->count; ++i) {
            if (scope->deferred[i] == object) return 1;
        }
    }

    if (dc && (GetCurrentObject(dc, OBJ_BRUSH) == object ||
               GetCurrentObject(dc, OBJ_PEN) == object)) {
        if (!scope || scope->count >= DECK_GDI_SCOPE_MAX) return 0;
        scope->deferred[scope->count++] = object;
        return 1;
    }

    return DeleteObject(object) ? 1 : 0;
}

unsigned int deck_gdi_scope_restore(HDC dc, DeckGdiScope *scope,
                                    HGDIOBJ previous_brush, HGDIOBJ previous_pen) {
    HGDIOBJ pending[DECK_GDI_SCOPE_MAX];
    unsigned int pending_count;
    unsigned int failures = 0;

    if (!dc || !scope || scope->count > DECK_GDI_SCOPE_MAX) return 1;

    pending_count = scope->count;
    if (pending_count != 0) {
        memcpy(pending, scope->deferred, pending_count * sizeof(pending[0]));
    }
    memset(scope->deferred, 0, sizeof(scope->deferred));
    scope->count = 0;

    if (previous_brush && previous_brush != HGDI_ERROR &&
        SelectObject(dc, previous_brush) == HGDI_ERROR) {
        ++failures;
    }
    if (previous_pen && previous_pen != HGDI_ERROR &&
        SelectObject(dc, previous_pen) == HGDI_ERROR) {
        ++failures;
    }

    for (unsigned int i = 0; i < pending_count; ++i) {
        HGDIOBJ object = pending[i];
        int duplicate = 0;

        if (!object || object == HGDI_ERROR) continue;
        for (unsigned int j = 0; j < i; ++j) {
            if (pending[j] == object) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) continue;

        if (GetCurrentObject(dc, OBJ_BRUSH) == object ||
            GetCurrentObject(dc, OBJ_PEN) == object ||
            !DeleteObject(object)) {
            /* Keep ownership so a later restore can retry after the DC state
               has been repaired. Never drop a handle that may still be live. */
            if (scope->count < DECK_GDI_SCOPE_MAX) {
                scope->deferred[scope->count++] = object;
            }
            ++failures;
        }
    }

    return failures;
}
