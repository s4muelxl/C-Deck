#ifndef CDECK_HOTKEYS_WIN32_H
#define CDECK_HOTKEYS_WIN32_H

#include "hotkeys.h"

/* Win32 callbacks for deck_hotkeys_apply; context is the owning HWND. */
int deck_hotkey_win32_register(void *context,
                               unsigned int id,
                               unsigned int modifiers,
                               unsigned int virtual_key,
                               unsigned long *error_code);
int deck_hotkey_win32_unregister(void *context,
                                 unsigned int id,
                                 unsigned long *error_code);

#endif
