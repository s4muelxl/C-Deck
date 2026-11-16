#ifndef CDECK_WIN_PATHS_H
#define CDECK_WIN_PATHS_H

#include "config.h"
#include <stddef.h>
#include <wchar.h>

/* Returns the unprefixed executable path, growing the Win32 query buffer as needed. */
int deck_get_module_path_wide(wchar_t *out, size_t capacity);

/* Builds config\apps.cfg beside a supplied module path using extended Win32 syntax. */
int deck_config_path_from_module_path(const wchar_t *module_path, char *out, size_t capacity);

/* Resolves the current executable path and returns its adjacent config\apps.cfg path as UTF-8. */
int deck_get_config_path_utf8(char *out, size_t capacity);

#endif
