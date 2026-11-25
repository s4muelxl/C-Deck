#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Isolate Win32 declarations from Raylib names and macros. */
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define LoadImage Win32LoadImage
#define LoadImageA Win32LoadImageA
#define DrawText Win32DrawText
#define DrawTextA Win32DrawTextA
#define DrawTextEx Win32DrawTextEx
#define DrawTextExA Win32DrawTextExA
#define PlaySound Win32PlaySound
#define PlaySoundA Win32PlaySoundA
#include <windows.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef LoadImageA
#undef DrawText
#undef DrawTextA
#undef DrawTextEx
#undef DrawTextExA
#undef PlaySound
#undef PlaySoundA
#ifdef COLOR_BACKGROUND
#undef COLOR_BACKGROUND
#endif

#include "raylib.h"
#include "interface.h"
#include "audio.h"
#include "apps.h"
#include "cdeck_icon_data.h"

static char g_last_graphics_error[512] = "";

/* Write a single UTF-8 timestamped line to %TEMP%\C-Deck-startup.log and debug output. */
void cdeck_log(const char *stage, const char *message) {
    char path[MAX_PATH];
    char line[2304];
    SYSTEMTIME st;
    HANDLE file;
    DWORD written;
    int len;

    if (stage == NULL) stage = "INFO";
    if (message == NULL) message = "";

    GetLocalTime(&st);
    len = snprintf(line, sizeof(line),
                   "[%04u-%02u-%02u %02u:%02u:%02u.%03u] [%s] %s\r\n",
                   (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
                   (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
                   (unsigned)st.wMilliseconds, stage, message);
    if (len <= 0) return;
    if ((size_t)len >= sizeof(line)) {
        len = (int)(sizeof(line) - 1);
    }

    OutputDebugStringA(line);

    if (GetTempPathA(MAX_PATH, path) == 0) return;
    if (strlen(path) + sizeof("C-Deck-startup.log") >= MAX_PATH) return;
    strcat(path, "C-Deck-startup.log");

    file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    WriteFile(file, line, (DWORD)len, &written, NULL);
    CloseHandle(file);
}

static const char *raylib_level_name(int log_level) {
    switch (log_level) {
        case LOG_TRACE: return "RAYLIB_TRACE";
        case LOG_DEBUG: return "RAYLIB_DEBUG";
        case LOG_INFO: return "RAYLIB_INFO";
        case LOG_WARNING: return "RAYLIB_WARN";
        case LOG_ERROR: return "RAYLIB_ERROR";
        case LOG_FATAL: return "RAYLIB_FATAL";
        default: return "RAYLIB";
    }
}

/* Persist Raylib/GLFW diagnostics so startup failures are actionable. */
static void cdeck_raylib_log(int log_level, const char *text, va_list args) {
    char message[2048];
    int formatted;

    formatted = vsnprintf(message, sizeof(message), text, args);
    if (formatted < 0) return;

    if (log_level >= LOG_WARNING || strstr(message, "GLFW: Error:") != NULL) {
        snprintf(g_last_graphics_error, sizeof(g_last_graphics_error), "%.511s", message);
    }
    cdeck_log(raylib_level_name(log_level), message);
}

/* Inspect WGL pixel format support before Raylib initializes GLFW/OpenGL. */
static void log_wgl_capabilities(void) {
    HDC hdc = GetDC(NULL);
    PIXELFORMATDESCRIPTOR pfd;
    int total = 0;
    int hardware = 0;
    int generic = 0;
    int i;
    char summary[256];

    if (hdc == NULL) {
        snprintf(summary, sizeof(summary), "GetDC(NULL) failed (error=%lu)", (unsigned long)GetLastError());
        cdeck_log("WGL_PROBE", summary);
        return;
    }

    memset(&pfd, 0, sizeof(pfd));
    total = DescribePixelFormat(hdc, 1, sizeof(pfd), &pfd);
    if (total <= 0) {
        snprintf(summary, sizeof(summary),
                 "DescribePixelFormat returned %d (error=%lu)", total, (unsigned long)GetLastError());
        cdeck_log("WGL_PROBE", summary);
        ReleaseDC(NULL, hdc);
        return;
    }

    for (i = 1; i <= total; ++i) {
        memset(&pfd, 0, sizeof(pfd));
        if (DescribePixelFormat(hdc, i, sizeof(pfd), &pfd) == 0) continue;
        if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL)) continue;
        if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT)) {
            generic++;
        } else {
            hardware++;
        }
    }

    ReleaseDC(NULL, hdc);
    snprintf(summary, sizeof(summary),
             "WGL pixel formats: total=%d, hardware_icd=%d, generic_software=%d",
             total, hardware, generic);
    cdeck_log("WGL_PROBE", summary);
}

static void show_window_error_dialog(void) {
    wchar_t detail[512];
    wchar_t message[1024];

    if (g_last_graphics_error[0] != '\0' &&
        MultiByteToWideChar(CP_UTF8, 0, g_last_graphics_error, -1,
                            detail, (int)(sizeof(detail) / sizeof(detail[0]))) > 0) {
        swprintf(message, sizeof(message) / sizeof(message[0]),
                 L"O C-Deck iniciou, mas o Raylib nao conseguiu criar a janela.\r\n\r\n"
                 L"Detalhe: %ls\r\n\r\n"
                 L"Consulte o arquivo C-Deck-startup.log na pasta temporaria do Windows.",
                 detail);
    } else {
        swprintf(message, sizeof(message) / sizeof(message[0]),
                 L"O C-Deck iniciou, mas o Raylib nao conseguiu criar a janela.\r\n"
                 L"Consulte o arquivo C-Deck-startup.log na pasta temporaria do Windows.");
    }

    MessageBoxW(NULL, message, L"C-Deck - Falha ao iniciar a interface", MB_OK | MB_ICONERROR);
}

/* Bind the multi-resolution Win32 .ico resource directly to titlebar, taskbar, and Alt+Tab. */
static void apply_window_icon(void) {
    HWND hwnd = (HWND)GetWindowHandle();
    HINSTANCE instance = GetModuleHandleW(NULL);
    int small_w = GetSystemMetrics(SM_CXSMICON);
    int small_h = GetSystemMetrics(SM_CYSMICON);
    int big_w = GetSystemMetrics(SM_CXICON);
    int big_h = GetSystemMetrics(SM_CYICON);
    HICON small_icon = NULL;
    HICON big_icon = NULL;

    if (hwnd == NULL || instance == NULL) return;

    small_icon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                   small_w, small_h, LR_SHARED);
    if (small_icon == NULL) {
        small_icon = (HICON)LoadImageW(instance, L"GLFW_ICON", IMAGE_ICON,
                                       small_w, small_h, LR_SHARED);
    }

    big_icon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                 big_w, big_h, LR_SHARED);
    if (big_icon == NULL) {
        big_icon = (HICON)LoadImageW(instance, L"GLFW_ICON", IMAGE_ICON,
                                     big_w, big_h, LR_SHARED);
    }

    if (small_icon != NULL) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small_icon);
        SetClassLongPtrW(hwnd, GCLP_HICONSM, (LONG_PTR)small_icon);
    }
    if (big_icon != NULL) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)big_icon);
        SetClassLongPtrW(hwnd, GCLP_HICON, (LONG_PTR)big_icon);
    }
}

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 640

static const Color COLOR_BACKGROUND = {13, 15, 19, 255};
static const Color COLOR_PANEL = {20, 23, 30, 255};
static const Color COLOR_BORDER = {40, 46, 56, 255};
static const Color COLOR_CARD = {26, 30, 38, 255};
static const Color COLOR_CARD_HOVER = {34, 40, 50, 255};
static const Color COLOR_TEXT = {244, 246, 248, 255};
static const Color COLOR_MUTED = {144, 154, 168, 255};
static const Color COLOR_ACCENT = {216, 222, 232, 255};
static const Color COLOR_SIGNATURE = {157, 255, 0, 255};
static const Color COLOR_TRACK = {46, 52, 64, 255};

typedef enum {
    ACTION_VOLUME_DOWN,
    ACTION_MUTE,
    ACTION_VOLUME_UP,
    ACTION_NOTEPAD,
    ACTION_CALCULATOR,
    ACTION_PAINT
} CDeckAction;

typedef struct {
    Rectangle bounds;
    const char *eyebrow;
    const char *title;
    const char *description;
    CDeckAction action;
} CDeckCard;

typedef struct {
    Font font;
    bool is_custom;
} UiFont;

static UiFont load_ui_font(const char *const *candidates, size_t count, int pixel_size) {
    UiFont result = {GetFontDefault(), false};
    char win_dir[MAX_PATH];
    char font_path[MAX_PATH];
    int codepoints[224];
    size_t i;

    if (GetWindowsDirectoryA(win_dir, MAX_PATH) == 0) {
        return result;
    }

    for (i = 0; i < 224; ++i) {
        codepoints[i] = 32 + (int)i;
    }

    for (i = 0; i < count; ++i) {
        Font loaded;
        if (snprintf(font_path, sizeof(font_path), "%s\\Fonts\\%s", win_dir, candidates[i]) >= MAX_PATH) {
            continue;
        }
        if (GetFileAttributesA(font_path) == INVALID_FILE_ATTRIBUTES) {
            continue;
        }
        loaded = LoadFontEx(font_path, pixel_size, codepoints, 224);
        if (IsFontValid(loaded) && loaded.texture.id != 0) {
            SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);
            result.font = loaded;
            result.is_custom = true;
            return result;
        }
        if (loaded.glyphs != NULL || loaded.recs != NULL) {
            UnloadFont(loaded);
        }
    }

    return result;
}

static void unload_ui_font(UiFont *ui_font) {
    if (ui_font != NULL && ui_font->is_custom) {
        UnloadFont(ui_font->font);
        ui_font->font = GetFontDefault();
        ui_font->is_custom = false;
    }
}

static Texture2D load_brand_icon_texture(void) {
    Texture2D texture = {0};
    Image icon_image = LoadImageFromMemory(".png", cdeck_ui_icon_png, (int)sizeof(cdeck_ui_icon_png));

    if (icon_image.data != NULL) {
        texture = LoadTextureFromImage(icon_image);
        UnloadImage(icon_image);
        if (texture.id != 0) {
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        }
    }
    return texture;
}

static void draw_brand_header_icon(Texture2D brand_icon, float x, float y, float size) {
    Rectangle dest = {x, y, size, size};

    if (brand_icon.id != 0) {
        Rectangle src = {0.0f, 0.0f, (float)CDECK_UI_ICON_DRAW_SIZE, (float)CDECK_UI_ICON_DRAW_SIZE};
        DrawTexturePro(brand_icon, src, dest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        return;
    }

    /* Vector fallback when PNG texture upload is unavailable. */
    DrawRectangleRounded(dest, 0.22f, 8, (Color){16, 19, 25, 255});
    DrawRectangleRoundedLines(dest, 0.22f, 8, COLOR_SIGNATURE);
    DrawRectangle((int)(x + 12), (int)(y + 11), (int)(size - 22), 5, COLOR_TEXT);
    DrawRectangle((int)(x + 12), (int)(y + 11), 5, (int)(size - 22), COLOR_TEXT);
    DrawRectangle((int)(x + 12), (int)(y + size - 16), (int)(size - 28), 5, COLOR_TEXT);
    DrawRectangle((int)(x + size - 19), (int)(y + size - 16), 9, 5, COLOR_SIGNATURE);
}

static float measure_ui_text(const UiFont *ui_font, const char *text, float size) {
    float spacing = ui_font->is_custom ? 0.5f : (size >= 20.0f ? 2.0f : 1.0f);
    return MeasureTextEx(ui_font->font, text, size, spacing).x;
}

static void draw_ui_text(const UiFont *ui_font, const char *text,
                         float x, float y, float size, Color color) {
    float spacing = ui_font->is_custom ? 0.5f : (size >= 20.0f ? 2.0f : 1.0f);
    DrawTextEx(ui_font->font, text, (Vector2){x, y}, size, spacing, color);
}

static void draw_ui_text_centered(const UiFont *ui_font, const char *text,
                                  float center_x, float y, float size, Color color) {
    float width = measure_ui_text(ui_font, text, size);
    draw_ui_text(ui_font, text, center_x - width * 0.5f, y, size, color);
}

static void draw_card(const CDeckCard *card, Vector2 mouse,
                      const UiFont *font_title, const UiFont *font_body) {
    bool hovered = CheckCollisionPointRec(mouse, card->bounds);
    Color fill = hovered ? COLOR_CARD_HOVER : COLOR_CARD;
    Color border = hovered ? (Color){157, 255, 0, 210} : COLOR_BORDER;
    float center_x = card->bounds.x + card->bounds.width * 0.5f;

    DrawRectangleRounded(card->bounds, 0.12f, 12, fill);
    DrawRectangleRoundedLines(card->bounds, 0.12f, 12, border);
    draw_ui_text(font_body, card->eyebrow, card->bounds.x + 20.0f,
                 card->bounds.y + 17.0f, 13.0f, COLOR_MUTED);
    draw_ui_text_centered(font_title, card->title, center_x,
                          card->bounds.y + 49.0f, 25.0f, COLOR_TEXT);
    draw_ui_text_centered(font_body, card->description, center_x,
                          card->bounds.y + 84.0f, 14.0f, COLOR_MUTED);
}

static void set_status(char *status, size_t capacity, const char *message) {
    snprintf(status, capacity, "%s", message);
}

static void run_action(CDeckAction action, char *status, size_t capacity) {
    BOOL ok = FALSE;

    switch (action) {
        case ACTION_VOLUME_DOWN:
            ok = cdeck_change_volume(-0.05f);
            set_status(status, capacity, ok ? "Volume reduzido em 5 pontos." : "Nao foi possivel alterar o volume.");
            break;
        case ACTION_MUTE:
            ok = cdeck_toggle_mute();
            set_status(status, capacity, ok ? "Estado do audio alterado." : "Nao foi possivel alterar o audio.");
            break;
        case ACTION_VOLUME_UP:
            ok = cdeck_change_volume(0.05f);
            set_status(status, capacity, ok ? "Volume aumentado em 5 pontos." : "Nao foi possivel alterar o volume.");
            break;
        case ACTION_NOTEPAD:
            ok = cdeck_open_application(L"notepad.exe");
            set_status(status, capacity, ok ? "Bloco de Notas iniciado." : "Nao foi possivel abrir o Bloco de Notas.");
            break;
        case ACTION_CALCULATOR:
            ok = cdeck_open_application(L"calc.exe");
            set_status(status, capacity, ok ? "Calculadora iniciada." : "Nao foi possivel abrir a Calculadora.");
            break;
        case ACTION_PAINT:
            ok = cdeck_open_application(L"mspaint.exe");
            set_status(status, capacity, ok ? "Paint iniciado." : "Nao foi possivel abrir o Paint.");
            break;
    }
}

int cdeck_run(void) {
    static const char *const BOLD_FONTS[] = {"segoeuib.ttf", "seguisb.ttf", "arialbd.ttf"};
    static const char *const BODY_FONTS[] = {"segoeui.ttf", "arial.ttf"};
    const Rectangle slider = {250, 190, 400, 8};
    const CDeckCard cards[6] = {
        {{48, 292, 254, 112}, "AUDIO / SYSTEM", "VOLUME -", "Diminuir em 5 pontos", ACTION_VOLUME_DOWN},
        {{323, 292, 254, 112}, "AUDIO / SYSTEM", "MUTE", "Alternar silencio", ACTION_MUTE},
        {{598, 292, 254, 112}, "AUDIO / SYSTEM", "VOLUME +", "Aumentar em 5 pontos", ACTION_VOLUME_UP},
        {{48, 424, 254, 112}, "QUICK LAUNCH / 01", "NOTEPAD", "Abrir Bloco de Notas", ACTION_NOTEPAD},
        {{323, 424, 254, 112}, "QUICK LAUNCH / 02", "CALCULATOR", "Abrir Calculadora", ACTION_CALCULATOR},
        {{598, 424, 254, 112}, "QUICK LAUNCH / 03", "PAINT", "Abrir Microsoft Paint", ACTION_PAINT}
    };
    UiFont font_title;
    UiFont font_body;
    Texture2D brand_icon;
    float volume = 0.0f;
    BOOL muted = FALSE;
    bool dragging_slider = false;
    char status[160] = "C-Deck pronto. Selecione uma acao.";
    int i;

    g_last_graphics_error[0] = '\0';
    log_wgl_capabilities();

    /* Avoid forcing multisampling: some Windows GPU/driver combinations reject it. */
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback(cdeck_raylib_log);
    SetConfigFlags(0);
    cdeck_log("UI", "Calling InitWindow(900x640)");
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C-Deck");
    if (!IsWindowReady()) {
        cdeck_log("ERROR", "InitWindow failed: IsWindowReady() returned false");
        show_window_error_dialog();
        return 2;
    }

    apply_window_icon();
    cdeck_log("UI", "Window ready; configuring min size and FPS");
    SetWindowMinSize(760, 580);
    SetTargetFPS(60);

    brand_icon = load_brand_icon_texture();
    font_title = load_ui_font(BOLD_FONTS, sizeof(BOLD_FONTS) / sizeof(BOLD_FONTS[0]), 26);
    font_body = load_ui_font(BODY_FONTS, sizeof(BODY_FONTS) / sizeof(BODY_FONTS[0]), 15);

    if (!cdeck_get_volume(&volume)) {
        volume = 0.0f;
        cdeck_log("AUDIO", "cdeck_get_volume failed; continuing with audio unavailable message");
        set_status(status, sizeof(status), "Audio indisponivel. Verifique o dispositivo padrao.");
    }
    cdeck_get_mute_state(&muted);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        Rectangle slider_hit = {slider.x - 8, slider.y - 18, slider.width + 16, 44};
        float next_volume;
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        const char *stack_badge = "C11  /  RAYLIB  /  WINDOWS";
        const char *esc_hint = "ESC  /  CLOSE";
        const char *vol_text;
        float badge_w;
        float esc_w;
        float vol_w;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, slider_hit)) {
            dragging_slider = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            dragging_slider = false;
        }
        if (dragging_slider && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            next_volume = (mouse.x - slider.x) / slider.width;
            if (next_volume < 0.0f) next_volume = 0.0f;
            if (next_volume > 1.0f) next_volume = 1.0f;
            if (next_volume - volume > 0.003f || volume - next_volume > 0.003f) {
                if (cdeck_set_volume(next_volume)) {
                    volume = next_volume;
                    set_status(status, sizeof(status), "Volume do sistema atualizado.");
                } else {
                    set_status(status, sizeof(status), "Falha ao atualizar o volume.");
                }
            }
        }

        for (i = 0; i < 6; ++i) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                CheckCollisionPointRec(mouse, cards[i].bounds)) {
                run_action(cards[i].action, status, sizeof(status));
                cdeck_get_volume(&volume);
                cdeck_get_mute_state(&muted);
            }
        }

        BeginDrawing();
        ClearBackground(COLOR_BACKGROUND);

        /* Header with embedded high-res C-Deck icon */
        DrawRectangle(0, 0, screen_w, 76, COLOR_PANEL);
        DrawLine(0, 76, screen_w, 76, COLOR_BORDER);
        draw_brand_header_icon(brand_icon, 48.0f, 16.0f, (float)CDECK_UI_ICON_DRAW_SIZE);
        draw_ui_text(&font_title, "C-DECK", 106.0f, 16.0f, 26.0f, COLOR_TEXT);
        draw_ui_text(&font_body, "DESKTOP CONTROL PANEL", 106.0f, 45.0f, 13.0f, COLOR_MUTED);

        badge_w = measure_ui_text(&font_body, stack_badge, 13.0f);
        DrawCircle((int)((float)screen_w - 48.0f - badge_w - 14.0f), 38, 3.5f, COLOR_SIGNATURE);
        draw_ui_text(&font_body, stack_badge, (float)screen_w - 48.0f - badge_w, 31.0f, 13.0f, COLOR_MUTED);

        /* System volume section */
        draw_ui_text(&font_body, "SYSTEM VOLUME", 48.0f, 122.0f, 14.0f, COLOR_MUTED);
        vol_text = TextFormat("%02.0f%%", volume * 100.0f);
        vol_w = measure_ui_text(&font_title, vol_text, 26.0f);
        draw_ui_text(&font_title, vol_text, (float)screen_w - 48.0f - vol_w, 114.0f, 26.0f, COLOR_TEXT);

        DrawCircle(52, 155, 3.5f, muted ? COLOR_MUTED : COLOR_SIGNATURE);
        draw_ui_text(&font_body, muted ? "OUTPUT MUTED" : "OUTPUT ACTIVE",
                     64.0f, 147.0f, 13.0f, muted ? COLOR_ACCENT : COLOR_MUTED);

        DrawRectangleRounded((Rectangle){slider.x, slider.y - 5, slider.width, 18},
                             0.5f, 8, COLOR_TRACK);
        if (volume > 0.01f) {
            DrawRectangleRounded((Rectangle){slider.x, slider.y - 5,
                                 slider.width * volume, 18},
                                 0.5f, 8, muted ? COLOR_MUTED : COLOR_ACCENT);
        }
        DrawCircle((int)(slider.x + slider.width * volume),
                   (int)(slider.y + 4), 10, COLOR_TEXT);
        DrawCircleLines((int)(slider.x + slider.width * volume),
                        (int)(slider.y + 4), 10, muted ? COLOR_BORDER : COLOR_SIGNATURE);

        /* Quick actions grid */
        draw_ui_text(&font_body, "QUICK ACTIONS", 48.0f, 250.0f, 14.0f, COLOR_MUTED);
        for (i = 0; i < 6; ++i) draw_card(&cards[i], mouse, &font_title, &font_body);

        /* Footer */
        DrawRectangle(0, screen_h - 48, screen_w, 48, COLOR_PANEL);
        DrawLine(0, screen_h - 48, screen_w, screen_h - 48, COLOR_BORDER);
        DrawCircle(52, screen_h - 24, 3.0f, COLOR_SIGNATURE);
        draw_ui_text(&font_body, status, 64.0f, (float)(screen_h - 32), 14.0f, COLOR_MUTED);
        esc_w = measure_ui_text(&font_body, esc_hint, 13.0f);
        draw_ui_text(&font_body, esc_hint, (float)screen_w - 48.0f - esc_w,
                     (float)(screen_h - 31), 13.0f, COLOR_MUTED);

        EndDrawing();
    }

    if (brand_icon.id != 0) {
        UnloadTexture(brand_icon);
    }
    unload_ui_font(&font_title);
    unload_ui_font(&font_body);

    cdeck_log("UI", "Window loop exited; closing Raylib window");
    CloseWindow();
    return 0;
}
