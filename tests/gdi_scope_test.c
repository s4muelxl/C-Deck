#include "../src/gdi_scope.h"
#include <assert.h>
#include <stdio.h>

static void test_stock_brush_selection_restores_previous_brush(HDC dc,
                                                                  HGDIOBJ original_brush) {
    HGDIOBJ previous;
    HGDIOBJ null_brush = GetStockObject(NULL_BRUSH);

    assert(null_brush != NULL && null_brush != HGDI_ERROR);
    previous = deck_gdi_select_stock_brush(dc, null_brush);
    assert(previous == original_brush);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == null_brush);
    assert(deck_gdi_restore_brush(dc, previous));
    assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);

    assert(deck_gdi_select_stock_brush(NULL, null_brush) == NULL);
    assert(!deck_gdi_restore_brush(NULL, original_brush));
    assert(!deck_gdi_restore_brush(dc, NULL));
    assert(!deck_gdi_restore_brush(dc, HGDI_ERROR));

    {
        HDC stale_dc = CreateCompatibleDC(NULL);
        assert(stale_dc != NULL);
        assert(DeleteDC(stale_dc));
        assert(!deck_gdi_select_object_checked(stale_dc, null_brush, NULL));
        assert(!deck_gdi_select_object_checked(dc, NULL, NULL));
        assert(!deck_gdi_select_object_checked(dc, HGDI_ERROR, NULL));
        assert(!deck_gdi_restore_brush(stale_dc, original_brush));
    }
}

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

    assert(brush != NULL);
    deck_gdi_scope_init(&scope);
    assert(SelectObject(dc, brush) != HGDI_ERROR);
    assert(deck_gdi_scope_release(dc, &scope, brush));
    assert(scope.count == 1);

    /* A missing HDC is a deterministic restore failure. The scope must keep
       ownership until the caller can retry with the valid drawing context. */
    assert(deck_gdi_scope_restore(NULL, &scope, original_brush, original_pen) > 0u);
    assert(scope.count == 1);
    assert(scope.deferred[0] == brush);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == brush);

    /* Once the caller supplies the valid previous brush, cleanup can retry. */
    assert(deck_gdi_scope_restore(dc, &scope, original_brush, original_pen) == 0u);
    assert(scope.count == 0);
    assert(GetCurrentObject(dc, OBJ_BRUSH) == original_brush);
}

static void test_restore_reports_invalid_hdc(HGDIOBJ original_brush,
                                                   HGDIOBJ original_pen) {
    HDC stale_dc = CreateCompatibleDC(NULL);
    DeckGdiScope scope;

    assert(stale_dc != NULL);
    assert(DeleteDC(stale_dc));
    deck_gdi_scope_init(&scope);

    /* A non-NULL but destroyed HDC makes SelectObject return an error
       sentinel. The restore API must report both failed selections. */
    assert(deck_gdi_scope_restore(stale_dc, &scope,
                                  original_brush, original_pen) >= 2u);
    assert(scope.count == 0);
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

        /* The first GDI operations may initialize process-local caches. */
        if (i == 0) before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    }

    /* Measure the repeated allocation/release loop separately from the
       targeted recovery fixtures below, each of which checks its own scope. */
    after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(after <= before + 1u);

    {
        DeckGdiScope scope;
        HBRUSH unselected = CreateSolidBrush(RGB(1, 2, 3));
        assert(unselected != NULL);
        deck_gdi_scope_init(&scope);
        assert(deck_gdi_scope_release(dc, &scope, unselected));
        assert(scope.count == 0);
    }

    test_stock_brush_selection_restores_previous_brush(dc, original_brush);
    test_duplicate_release_is_queued_once(dc, original_brush, original_pen);
    test_queued_handle_remains_owned_after_external_restore(dc, original_brush, original_pen);
    test_failed_restore_retains_handle_for_retry(dc, original_brush, original_pen);
    test_restore_reports_invalid_hdc(original_brush, original_pen);

    DeleteDC(dc);

    puts("GDI selected-object restore, retry and resource lifetime: OK");
    return 0;
}
