#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Evita conflitos de nomes e macros da Win32 API com simbolos da Raylib. */
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

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 640

static const Color COLOR_BACKGROUND = {18, 18, 20, 255};
static const Color COLOR_PANEL = {29, 29, 33, 255};
static const Color COLOR_CARD = {36, 36, 41, 255};
static const Color COLOR_CARD_HOVER = {48, 48, 54, 255};
static const Color COLOR_TEXT = {240, 240, 242, 255};
static const Color COLOR_MUTED = {145, 145, 153, 255};
static const Color COLOR_ACCENT = {205, 205, 212, 255};
static const Color COLOR_TRACK = {67, 67, 75, 255};

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

static char g_last_graphics_error[512] = "";

/* Registra eventos em UTF-8 no log de inicializacao (%TEMP%\C-Deck-startup.log). */
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

/* Consulta formatos de pixel WGL disponiveis antes de abrir a janela Raylib/GLFW. */
static void log_wgl_capabilities(void) {
    HDC hdc = GetDC(NULL);
    PIXELFORMATDESCRIPTOR pfd;
    int total = 0;
    int hardware = 0;
    int generic = 0;
    int i;
    char summary[256];

    if (hdc == NULL) {
        snprintf(summary, sizeof(summary), "Falha em GetDC(NULL) (erro=%lu)", (unsigned long)GetLastError());
        cdeck_log("WGL_PROBE", summary);
        return;
    }

    memset(&pfd, 0, sizeof(pfd));
    total = DescribePixelFormat(hdc, 1, sizeof(pfd), &pfd);
    if (total <= 0) {
        snprintf(summary, sizeof(summary),
                 "DescribePixelFormat retornou %d (erro=%lu)", total, (unsigned long)GetLastError());
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
             "Formatos WGL: total=%d, hardware_icd=%d, software_generico=%d",
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
                 L"O C-Deck iniciou, mas nao foi possivel criar a janela grafica.\r\n\r\n"
                 L"Detalhe: %ls\r\n\r\n"
                 L"Consulte o arquivo C-Deck-startup.log na pasta temporaria do Windows.",
                 detail);
    } else {
        swprintf(message, sizeof(message) / sizeof(message[0]),
                 L"O C-Deck iniciou, mas nao foi possivel criar a janela grafica.\r\n"
                 L"Consulte o arquivo C-Deck-startup.log na pasta temporaria do Windows.");
    }

    MessageBoxW(NULL, message, L"C-Deck - Falha ao iniciar a interface", MB_OK | MB_ICONERROR);
}

static void draw_text_centered(const char *text, int center_x, int y,
                               int font_size, Color color) {
    DrawText(text, center_x - MeasureText(text, font_size) / 2,
             y, font_size, color);
}

static void draw_card(const CDeckCard *card, Vector2 mouse) {
    bool hovered = CheckCollisionPointRec(mouse, card->bounds);
    Color fill = hovered ? COLOR_CARD_HOVER : COLOR_CARD;
    int center_x = (int)(card->bounds.x + card->bounds.width / 2.0f);

    DrawRectangleRounded(card->bounds, 0.12f, 12, fill);
    DrawRectangleRoundedLines(card->bounds, 0.12f, 12,
                              hovered ? COLOR_ACCENT : (Color){58, 58, 65, 255});
    DrawText(card->eyebrow, (int)card->bounds.x + 22,
             (int)card->bounds.y + 18, 12, COLOR_MUTED);
    draw_text_centered(card->title, center_x,
                       (int)card->bounds.y + 53, 25, COLOR_TEXT);
    draw_text_centered(card->description, center_x,
                       (int)card->bounds.y + 91, 13, COLOR_MUTED);
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
    const Rectangle slider = {250, 190, 400, 8};
    const CDeckCard cards[6] = {
        {{48, 292, 254, 112}, "AUDIO / SYSTEM", "VOLUME -", "Diminuir em 5 pontos", ACTION_VOLUME_DOWN},
        {{323, 292, 254, 112}, "AUDIO / SYSTEM", "MUTE", "Alternar silencio", ACTION_MUTE},
        {{598, 292, 254, 112}, "AUDIO / SYSTEM", "VOLUME +", "Aumentar em 5 pontos", ACTION_VOLUME_UP},
        {{48, 424, 254, 112}, "QUICK LAUNCH / 01", "NOTEPAD", "Abrir Bloco de Notas", ACTION_NOTEPAD},
        {{323, 424, 254, 112}, "QUICK LAUNCH / 02", "CALCULATOR", "Abrir Calculadora", ACTION_CALCULATOR},
        {{598, 424, 254, 112}, "QUICK LAUNCH / 03", "PAINT", "Abrir Microsoft Paint", ACTION_PAINT}
    };
    float volume = 0.0f;
    BOOL muted = FALSE;
    bool dragging_slider = false;
    char status[160] = "C-Deck pronto. Selecione uma acao.";
    int i;

    g_last_graphics_error[0] = '\0';
    log_wgl_capabilities();

    /* Nao exigir MSAA evita rejeicao de pixel format em drivers integrados e VMs. */
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback(cdeck_raylib_log);
    SetConfigFlags(0);
    cdeck_log("INTERFACE", "Executando InitWindow(900x640)");
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C-Deck");
    if (!IsWindowReady()) {
        cdeck_log("ERRO", "InitWindow falhou: IsWindowReady() retornou falso");
        show_window_error_dialog();
        return 2;
    }

    cdeck_log("INTERFACE", "Janela criada; configurando tamanho minimo e FPS");
    SetWindowMinSize(760, 580);
    SetTargetFPS(60);

    if (!cdeck_get_volume(&volume)) {
        volume = 0.0f;
        cdeck_log("AUDIO", "Dispositivo padrao de audio indisponivel na inicializacao");
        set_status(status, sizeof(status), "Audio indisponivel. Verifique o dispositivo padrao.");
    }
    cdeck_get_mute_state(&muted);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        Rectangle slider_hit = {slider.x - 8, slider.y - 18, slider.width + 16, 44};
        float next_volume;

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

        DrawRectangle(0, 0, GetScreenWidth(), 76, COLOR_PANEL);
        DrawText("C-DECK", 48, 22, 27, COLOR_TEXT);
        DrawText("DESKTOP CONTROL PANEL", 164, 31, 11, COLOR_MUTED);
        DrawText("C11  /  RAYLIB  /  WINDOWS", GetScreenWidth() - 270, 32, 11, COLOR_MUTED);

        DrawText("SYSTEM VOLUME", 48, 125, 13, COLOR_MUTED);
        DrawText(TextFormat("%02.0f%%", volume * 100.0f),
                 GetScreenWidth() - 112, 116, 25, COLOR_TEXT);
        DrawText(muted ? "OUTPUT MUTED" : "OUTPUT ACTIVE",
                 48, 151, 12, muted ? COLOR_ACCENT : COLOR_MUTED);

        DrawRectangleRounded((Rectangle){slider.x, slider.y - 5, slider.width, 18},
                             0.5f, 8, COLOR_TRACK);
        DrawRectangleRounded((Rectangle){slider.x, slider.y - 5,
                             slider.width * volume, 18},
                             0.5f, 8, COLOR_ACCENT);
        DrawCircle((int)(slider.x + slider.width * volume),
                   (int)(slider.y + 4), 10, COLOR_TEXT);

        DrawText("QUICK ACTIONS", 48, 252, 13, COLOR_MUTED);
        for (i = 0; i < 6; ++i) draw_card(&cards[i], mouse);

        DrawRectangle(0, GetScreenHeight() - 48, GetScreenWidth(), 48, COLOR_PANEL);
        DrawText(status, 48, GetScreenHeight() - 31, 12, COLOR_MUTED);
        DrawText("ESC  /  CLOSE", GetScreenWidth() - 150,
                 GetScreenHeight() - 31, 11, COLOR_MUTED);

        EndDrawing();
    }

    cdeck_log("INTERFACE", "Loop principal encerrado; fechando janela Raylib");
    CloseWindow();
    return 0;
}
