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

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C-Deck");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(760, 580);
    SetTargetFPS(60);

    if (!cdeck_get_volume(&volume)) {
        volume = 0.0f;
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

    CloseWindow();
    return 0;
}
