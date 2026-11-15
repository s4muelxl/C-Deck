#include "../src/action_library.h"
#include "../src/config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const size_t expected[DECK_LIBRARY_CATEGORY_COUNT] = { 4, 3, 4, 3, 3, 3, 0 };

    for (int category = 0; category < DECK_LIBRARY_CATEGORY_COUNT; ++category) {
        size_t count = action_library_count((DeckLibraryCategory)category);
        assert(count == expected[category]);
        for (size_t i = 0; i < count; ++i) {
            const DeckLibraryAction *action = action_library_get((DeckLibraryCategory)category, i);
            assert(action != NULL);
            assert(action->category == (DeckLibraryCategory)category);
            assert(action->title != NULL && action->title[0] != L'\\0');
            assert(action->name != NULL && action->name[0] != '\\0');
            assert(action->path != NULL && action->path[0] != '\\0');
            assert(strlen(action->name) < DECK_TEXT_MAX);
            assert(strlen(action->path) < DECK_TEXT_MAX);
            assert(strchr(action->name, '\\n') == NULL);
            assert(strchr(action->path, '\\n') == NULL);
            assert(action->icon_path != NULL && strncmp(action->icon_path, "icon:", 5) == 0);
        }
        assert(action_library_get((DeckLibraryCategory)category, count) == NULL);
    }

    assert(action_library_count((DeckLibraryCategory)-1) == 0);
    assert(action_library_count((DeckLibraryCategory)DECK_LIBRARY_CATEGORY_COUNT) == 0);
    assert(action_library_get(DECK_LIBRARY_MULTIACTION, 0) == NULL);

    puts("action library catalog: OK");
    return 0;
}
