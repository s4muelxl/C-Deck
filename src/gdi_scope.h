#ifndef CDECK_GDI_SCOPE_H
#define CDECK_GDI_SCOPE_H

#include <windows.h>

#define DECK_GDI_SCOPE_MAX 64

typedef struct DeckGdiScope {
    HGDIOBJ deferred[DECK_GDI_SCOPE_MAX];
    unsigned int count;
} DeckGdiScope;

void deck_gdi_scope_init(DeckGdiScope *scope);
/* Returns nonzero when the object was deleted or safely queued for restoration. */
int deck_gdi_scope_release(HDC dc, DeckGdiScope *scope, HGDIOBJ object);
/* Returns the number of deferred DeleteObject calls that failed. */
unsigned int deck_gdi_scope_restore(HDC dc, DeckGdiScope *scope,
                                    HGDIOBJ previous_brush, HGDIOBJ previous_pen);

#endif
