#include "../src/gdi_scope.h"
#include <assert.h>
#include <stdio.h>

static void test_duplicate_release_is_queued_once(HDC dc,
                                                   HGDIOBJ original_brush,
                                                   HGDIOBJ original_pen) {
    DeckGdiScope scope;
    HBRUSH brush = CreateSolidBrush(RGB(9, 17, 25));

    assert(brush != NULL);
    deck_gdi_scope_init(&scope);
    assert(SelectObject(dc, brush) != HGDI_ERROR);
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(scope.count == 1);

    assert(deck_gdi_scope_restore(dc, &scope, original_brush, original_pen) == 0u);
    assert(scope.count == 0);
    /* Successful restore plus an empty deferred list proves ownership was released.
       Do not query a freed HGDIOBJ: Windows may recycle its numeric handle. */
    assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);
}

static void test_queued_handle_remains_owned_after_external_restore(
    HDC dc, HGDIOBJ original_brush, HGDIOBJ original_pen) {
    DeckGdiScope scope;
    HBRUSH brush = CreateSolidBrush(RGB(21, 33, 45));

    assert(brush != NULL);
    deck_gdi_scope_init(&scope);
    assert(SelectObject(dc, brush) != HGDI_ERROR);
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(scope.count == 1);

    /* Simulate a caller restoring the HDC before asking to release again. */
    assert(SelectObject(dc, original_brush) == brush);
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(scope.count == 1);
    assert(scope.deferred[0] == brush);

    assert(deck_gdi_scope_restore(dc, &scope, original_brush, original_pen) == 0u);
    assert(scope.count == 0);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);
}

static void test_failed_restore_retains_handle_for_retry(HDC dc,
                                                          HGDIOBJ original_brush,
                                                          HGDIOBJ original_pen) {
    DeckGdiScope scope;
    HBRUSH brush = CreateSolidBrush(RGB(31, 47, 63));
    HBRUSH stale_previous = CreateSolidBrush(RGB(2, 4, 8));

    assert(brush != NULL && stale_previous != NULL);
    assert(DeleteObject(stale_previous));
    deck_gdi_scope_init(&scope);
    assert(SelectObject(dc, brush) != HGDI_ERROR);
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(scope.count == 1);

    /* A deleted GDI handle cannot restore the original brush. The selected
       object must remain owned by the scope instead of being forgotten. */
    assert(deck_gdi_scope_restore(dc, &scope, stale_previous, original_pen) > 0u);
    assert(scope.count == 1);
    assert(scope.deferred[0] == brush);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == brush);
    assert(!DeleteObject(brush));

    /* Once the caller supplies the valid previous brush, cleanup can retry. */
    assert(deck_gdi_scope_restore(dc, &scope, original_brush, original_pen) == 0u);
    assert(scope.count == 0);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);
}

int main(void) {
    HDC dc = CreateCompatibleDC(NULL);
    HGDIOBJ original_brush;
    HGDIOBJ original_pen;
    DWORD before;
    DWORD after;

    assert(dc != NULL);
    original_brush = GetCurrentObject(dc, OBJ_BRUSH);
    original_pen = GetCurrentObject(dc, OBJ_PEN);
    assert(original_brush != NULL && original_brush != HGDI_ERROR);
    assert(original_pen != NULL && original_pen != HGDI_ERROR);

    before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);

    for (unsigned int i = 0; i < 500; ++i) {
        DeckGdiScope scope;
        HBRUSH brush = CreateSolidBrush(RGB((i * 13u) & 0xffu,
                                             (i * 29u) & 0xffu,
                                             (i * 47u) & 0xffu));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB((i * 7u) & 0xffu,
                                              (i * 17u) & 0xffu,
                                              (i * 31u) & 0xffu));
        assert(brush != NULL && pen != NULL);

        deck_gdi_scope_init(&scope);
        assert(SelectObject(dc, brush) != HGDI_ERROR);
        assert(SelectObject(dc, pen) != HGDI_ERROR);

        assert(deck_gdi_scope_release(dc, &scope, brush));
        assert(deck_gdi_scope_release(dc, &scope, pen));
        assert(scope.count == 2);
        assert(GetObjectType(brush) == OBJ_BRUSH);
        assert(GetObjectType(pen) == OBJ_PEN);

        assert(deck_gdi_scope_restore(dc, &scope, original_brush, original_pen) == 0u);
        assert(scope.count == 0);
        assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);
        assert(GetCurrentObject(dc, OBJ_PEN) == original_pen);
    }

    {
        DeckGdiScope scope;
        HBRUSH unselected = CreateSolidBrush(RGB(1, 2, 3));
        assert(unselected != NULL);
        deck_gdi_scope_init(&scope);
        assert(deck_gdi_scope_release(dc, &scope, unselected));
        assert(scope.count == 0);
    }

    test_duplicate_release_is_queued_once(dc, original_brush, original_pen);
    test_failed_restore_retains_handle_for_retry(dc, original_brush, original_pen);

    after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(after <= before + 1u);
    DeleteDC(dc);

    puts("GDI selected-object restore, retry and resource lifetime: OK");
    return 0;
}
