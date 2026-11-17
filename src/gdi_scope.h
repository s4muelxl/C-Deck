#ifndef CDECK_GDI_SCOPE_H
#define CDECK_GDI_SCOPE_H

#include <windows.h>

#define DECK_GDI_SCOPE_MAX 64

typedef struct DeckGdiScope {
    HGDIOBJ deferred[DECK_GDI_SCOPE_MAX];
    unsigned int count;
} DeckGdiScope;

void deck_gdi_scope_init(DeckGdiScope *scope);
/* Selects an object and validates both SelectObject failure sentinels. */
int deck_gdi_select_object_checked(HDC dc, HGDIOBJ object, HGDIOBJ *previous_object);
/* Selects an owned object and queues it for release when the scope is restored. */
int deck_gdi_scope_select_owned(HDC dc, DeckGdiScope *scope,
                                HGDIOBJ object, HGDIOBJ *previous_object);
/* Selects a stock brush and returns the previous brush, or NULL on failure. */
HGDIOBJ deck_gdi_select_stock_brush(HDC dc, HGDIOBJ stock_brush);
/* Restores a previously selected brush without taking ownership of it. */
int deck_gdi_restore_brush(HDC dc, HGDIOBJ previous_brush);
/* Returns nonzero when deleted or queued once for deferred restoration. */
int deck_gdi_scope_release(HDC dc, DeckGdiScope *scope, HGDIOBJ object);
/* Returns failures; handles still selected or not deletable remain queued for retry. */
unsigned int deck_gdi_scope_restore(HDC dc, DeckGdiScope *scope,
                                    HGDIOBJ previous_brush, HGDIOBJ previous_pen);

#endif
