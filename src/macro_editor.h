#ifndef CDECK_MACRO_EDITOR_H
#define CDECK_MACRO_EDITOR_H

#include <windows.h>
#include "macros.h"

/* Returns 1 when saved, 0 when cancelled, and -1 when the editor cannot open. */
int macro_editor_show(HWND parent, DeckMacroStore *store);

#endif
