#ifndef CDECK_ACTION_LIBRARY_H
#define CDECK_ACTION_LIBRARY_H

#include <stddef.h>
#include <wchar.h>

typedef enum DeckLibraryCategory {
    DECK_LIBRARY_FAVORITES = 0,
    DECK_LIBRARY_SYSTEM,
    DECK_LIBRARY_AUDIO,
    DECK_LIBRARY_MEDIA,
    DECK_LIBRARY_APPS,
    DECK_LIBRARY_NAVIGATION,
    DECK_LIBRARY_MULTIACTION,
    DECK_LIBRARY_CATEGORY_COUNT
} DeckLibraryCategory;

typedef struct DeckLibraryAction {
    DeckLibraryCategory category;
    const wchar_t *title;
    const char *name;
    const char *path;
    const char *icon_path;
} DeckLibraryAction;

/* Returns immutable templates for the selected category. */
size_t action_library_count(DeckLibraryCategory category);
const DeckLibraryAction *action_library_get(DeckLibraryCategory category, size_t index);

#endif
