#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#include <windows.h>
#undef Rectangle
#undef CloseWindow
#ifdef DrawText
#undef DrawText
#endif

#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include "interface.h"
#include "audio.h"
#include "apps.h"

/* Existing Raylib UI implementation follows unchanged. */
