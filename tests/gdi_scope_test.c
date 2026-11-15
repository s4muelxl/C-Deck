#include "../src/gdi_scope.h"
#include <assert.h>
#include <stdio.h>

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

    after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(after <= before + 1u);
    DeleteDC(dc);

    puts("GDI selected-object restore and resource lifetime: OK");
    return 0;
}
