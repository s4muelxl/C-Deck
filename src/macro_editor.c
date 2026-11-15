#include "macro_editor.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#define EDITOR_CLASS_NAME       L"CDeckMacroEditorWindow"
#define EDITOR_WIDTH            800
#define EDITOR_HEIGHT           640

#define ID_EDITOR_MACRO_COMBO   6101
#define ID_EDITOR_NAME          6102
#define ID_EDITOR_STEPS         6103
#define ID_EDITOR_STEP_TYPE     6104
#define ID_EDITOR_STEP_VALUE    6105
#define ID_EDITOR_ADD_STEP      6106
#define ID_EDITOR_UPDATE_STEP   6107
#define ID_EDITOR_REMOVE_STEP   6108
#define ID_EDITOR_STEP_UP       6109
#define ID_EDITOR_STEP_DOWN     6110
#define ID_EDITOR_NEW_MACRO     6111
#define ID_EDITOR_SAVE          6112
#define ID_EDITOR_CANCEL        6113
#define ID_EDITOR_STATUS        6114

typedef struct MacroEditorContext {
    DeckMacroStore working;
    DeckMacroStore *destination;
    HWND macro_combo;
    HWND name_edit;
    HWND steps_list;
    HWND step_type;
    HWND step_value;
    HWND status;
    HFONT font;
    HBRUSH background;
    HBRUSH input_background;
    unsigned int slot;
    unsigned int step_count;
    DeckMacroStep steps[DECK_MACRO_STEP_COUNT];
    int result;
} MacroEditorContext;

static int editor_utf8_to_wide(const char *source, wchar_t *destination, int capacity) {
    if (!source || !destination || capacity <= 0) return 0;
    destination[0] = L'\0';
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1,
                               destination, capacity) > 0;
}

static int editor_wide_to_utf8(const wchar_t *source, char *destination, size_t capacity) {
    int written;
    if (!source || !destination || capacity == 0 || capacity > INT_MAX) return 0;
    destination[0] = '\0';
    written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source, -1,
                                  destination, (int)capacity, NULL, NULL);
    return written > 0;
}

static void editor_set_status(MacroEditorContext *context, const wchar_t *message) {
    if (context && context->status) SetWindowTextW(context->status, message ? message : L"");
}

static HWND editor_create_control(HWND parent, MacroEditorContext *context,
                                  const wchar_t *class_name, const wchar_t *text,
                                  DWORD style, DWORD ex_style,
                                  int x, int y, int width, int height, int control_id) {
    HWND control = CreateWindowExW(
        ex_style, class_name, text, style,
        x, y, width, height, parent,
        control_id ? (HMENU)(INT_PTR)control_id : NULL,
        GetModuleHandleW(NULL), NULL);
    if (control && context->font) SendMessageW(control, WM_SETFONT, (WPARAM)context->font, TRUE);
    return control;
}

static void editor_refresh_macro_slots(MacroEditorContext *context) {
    SendMessageW(context->macro_combo, CB_RESETCONTENT, 0, 0);
    for (unsigned int i = 0; i < DECK_MACRO_COUNT; ++i) {
        const DeckMacro *macro = macro_store_get(&context->working, i + 1);
        wchar_t label[DECK_MACRO_NAME_MAX * 2 + 48];
        wchar_t name[DECK_MACRO_NAME_MAX * 2];
        if (macro && editor_utf8_to_wide(macro->name, name, (int)(sizeof(name) / sizeof(name[0])))) {
            swprintf(label, sizeof(label) / sizeof(label[0]), L"Macro %u — %ls", i + 1, name);
        } else {
            swprintf(label, sizeof(label) / sizeof(label[0]), L"Macro %u — vazio", i + 1);
        }
        SendMessageW(context->macro_combo, CB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageW(context->macro_combo, CB_SETCURSEL, context->slot, 0);
}

static void editor_format_step(const DeckMacroStep *step, unsigned int index,
                               wchar_t *buffer, size_t capacity) {
    wchar_t value[DECK_MACRO_VALUE_MAX * 2];
    if (!step || !buffer || capacity == 0) return;
    value[0] = L'\0';
    if (step->type == DECK_MACRO_STEP_ACTION) {
        editor_utf8_to_wide(step->value, value, (int)(sizeof(value) / sizeof(value[0])));
        swprintf(buffer, capacity, L"%02u  Ação  ·  %ls", index + 1, value);
    } else if (step->type == DECK_MACRO_STEP_OPEN) {
        editor_utf8_to_wide(step->value, value, (int)(sizeof(value) / sizeof(value[0])));
        swprintf(buffer, capacity, L"%02u  Abrir  ·  %ls", index + 1, value);
    } else if (step->type == DECK_MACRO_STEP_DELAY) {
        swprintf(buffer, capacity, L"%02u  Aguardar  ·  %u ms", index + 1, step->delay_ms);
    } else {
        swprintf(buffer, capacity, L"%02u  Etapa inválida", index + 1);
    }
}

static void editor_refresh_steps(MacroEditorContext *context, int selected_index) {
    SendMessageW(context->steps_list, LB_RESETCONTENT, 0, 0);
    for (unsigned int i = 0; i < context->step_count; ++i) {
        wchar_t label[DECK_MACRO_VALUE_MAX * 2 + 48];
        editor_format_step(&context->steps[i], i, label, sizeof(label) / sizeof(label[0]));
        SendMessageW(context->steps_list, LB_ADDSTRING, 0, (LPARAM)label);
    }
    if (selected_index >= 0 && (unsigned int)selected_index < context->step_count) {
        SendMessageW(context->steps_list, LB_SETCURSEL, (WPARAM)selected_index, 0);
    }
}

static void editor_load_slot(MacroEditorContext *context, unsigned int slot) {
    const DeckMacro *macro;
    wchar_t name[DECK_MACRO_NAME_MAX * 2];
    context->slot = slot;
    context->step_count = 0;
    ZeroMemory(context->steps, sizeof(context->steps));
    macro = macro_store_get(&context->working, slot + 1);
    name[0] = L'\0';
    if (macro) {
        editor_utf8_to_wide(macro->name, name, (int)(sizeof(name) / sizeof(name[0])));
        context->step_count = macro->step_count;
        memcpy(context->steps, macro->steps, sizeof(context->steps));
    }
    SetWindowTextW(context->name_edit, name);
    SendMessageW(context->step_type, CB_SETCURSEL, 0, 0);
    SetWindowTextW(context->step_value, L"");
    editor_refresh_macro_slots(context);
    editor_refresh_steps(context, -1);
    editor_set_status(context, macro ? L"Edite as etapas e salve para aplicar." :
                                      L"Slot livre. Crie uma macro e adicione pelo menos uma etapa.");
}

static int editor_capture_current(MacroEditorContext *context) {
    wchar_t wide_name[DECK_MACRO_NAME_MAX * 2];
    char name[DECK_MACRO_NAME_MAX];
    DeckMacro candidate;
    DeckMacro *target;

    if (!context || context->slot >= DECK_MACRO_COUNT) return 0;
    GetWindowTextW(context->name_edit, wide_name, (int)(sizeof(wide_name) / sizeof(wide_name[0])));
    if (!editor_wide_to_utf8(wide_name, name, sizeof(name))) {
        editor_set_status(context, L"O nome excede o limite UTF-8 de 63 bytes ou contém texto inválido.");
        return 0;
    }

    target = &context->working.macros[context->slot];
    if (!name[0] && context->step_count == 0) {
        if (target->configured) {
            editor_set_status(context, L"Macros existentes não podem ser excluídas pelo editor; mantenha um nome e ao menos uma etapa.");
            return 0;
        }
        ZeroMemory(target, sizeof(*target));
        return 1;
    }
    if (!name[0] || context->step_count == 0) {
        editor_set_status(context, L"Informe um nome e adicione pelo menos uma etapa.");
        return 0;
    }

    ZeroMemory(&candidate, sizeof(candidate));
    memcpy(candidate.name, name, strlen(name) + 1);
    candidate.configured = 1;
    candidate.step_count = context->step_count;
    memcpy(candidate.steps, context->steps, sizeof(candidate.steps));
    if (!macro_validate(&candidate)) {
        editor_set_status(context, L"A macro contém uma etapa inválida ou excede os limites permitidos.");
        return 0;
    }
    *target = candidate;
    return 1;
}

static void editor_load_selected_step(MacroEditorContext *context) {
    LRESULT selected = SendMessageW(context->steps_list, LB_GETCURSEL, 0, 0);
    const DeckMacroStep *step;
    wchar_t value[DECK_MACRO_VALUE_MAX * 2];
    if (selected == LB_ERR || (unsigned long)selected >= context->step_count) return;
    step = &context->steps[selected];
    value[0] = L'\0';
    if (step->type == DECK_MACRO_STEP_ACTION) {
        SendMessageW(context->step_type, CB_SETCURSEL, 0, 0);
        editor_utf8_to_wide(step->value, value, (int)(sizeof(value) / sizeof(value[0])));
    } else if (step->type == DECK_MACRO_STEP_OPEN) {
        SendMessageW(context->step_type, CB_SETCURSEL, 1, 0);
        editor_utf8_to_wide(step->value, value, (int)(sizeof(value) / sizeof(value[0])));
    } else if (step->type == DECK_MACRO_STEP_DELAY) {
        SendMessageW(context->step_type, CB_SETCURSEL, 2, 0);
        swprintf(value, sizeof(value) / sizeof(value[0]), L"%u", step->delay_ms);
    }
    SetWindowTextW(context->step_value, value);
}

static int editor_parse_step(MacroEditorContext *context, DeckMacroStep *step) {
    wchar_t wide_value[DECK_MACRO_VALUE_MAX * 2];
    char value[DECK_MACRO_VALUE_MAX];
    char expression[DECK_MACRO_VALUE_MAX + 16];
    int type;
    int written;

    if (!context || !step) return 0;
    type = (int)SendMessageW(context->step_type, CB_GETCURSEL, 0, 0);
    if (type < 0 || type > 2) {
        editor_set_status(context, L"Selecione o tipo da etapa.");
        return 0;
    }
    GetWindowTextW(context->step_value, wide_value,
                   (int)(sizeof(wide_value) / sizeof(wide_value[0])));
    if (!editor_wide_to_utf8(wide_value, value, sizeof(value)) || !value[0]) {
        editor_set_status(context, L"Informe o valor da etapa.");
        return 0;
    }

    if (type == 0) written = snprintf(expression, sizeof(expression), "action:%s", value);
    else if (type == 1) written = snprintf(expression, sizeof(expression), "open:%s", value);
    else written = snprintf(expression, sizeof(expression), "delay:%s", value);
    if (written < 0 || (size_t)written >= sizeof(expression) ||
        !macro_step_parse(expression, step)) {
        editor_set_status(context, L"Valor inválido. Ações, destinos e atrasos seguem a lista permitida.");
        return 0;
    }
    return 1;
}

static void editor_add_step(MacroEditorContext *context) {
    DeckMacroStep step;
    DeckMacro draft;
    if (context->step_count >= DECK_MACRO_STEP_COUNT) {
        editor_set_status(context, L"Esta macro já atingiu o limite de 12 etapas.");
        return;
    }
    if (!editor_parse_step(context, &step)) return;
    ZeroMemory(&draft, sizeof(draft));
    draft.step_count = context->step_count;
    memcpy(draft.steps, context->steps, sizeof(draft.steps));
    if (!macro_step_insert(&draft, draft.step_count, &step)) {
        editor_set_status(context, L"Não foi possível adicionar a etapa.");
        return;
    }
    context->step_count = draft.step_count;
    memcpy(context->steps, draft.steps, sizeof(context->steps));
    editor_refresh_steps(context, (int)context->step_count - 1);
    editor_set_status(context, L"Etapa adicionada. Salve a macro para persistir as alterações.");
}

static void editor_update_step(MacroEditorContext *context) {
    LRESULT selected = SendMessageW(context->steps_list, LB_GETCURSEL, 0, 0);
    DeckMacroStep step;
    DeckMacro draft;
    if (selected == LB_ERR || (unsigned long)selected >= context->step_count) {
        editor_set_status(context, L"Selecione uma etapa para atualizar.");
        return;
    }
    if (!editor_parse_step(context, &step)) return;
    ZeroMemory(&draft, sizeof(draft));
    draft.step_count = context->step_count;
    memcpy(draft.steps, context->steps, sizeof(draft.steps));
    if (!macro_step_replace(&draft, (unsigned int)selected, &step)) {
        editor_set_status(context, L"Não foi possível atualizar a etapa.");
        return;
    }
    memcpy(context->steps, draft.steps, sizeof(context->steps));
    editor_refresh_steps(context, (int)selected);
    editor_set_status(context, L"Etapa atualizada. Salve a macro para persistir as alterações.");
}

static void editor_remove_step(MacroEditorContext *context) {
    LRESULT selected = SendMessageW(context->steps_list, LB_GETCURSEL, 0, 0);
    DeckMacro draft;
    int next_index;
    if (selected == LB_ERR || (unsigned long)selected >= context->step_count) {
        editor_set_status(context, L"Selecione uma etapa para remover.");
        return;
    }
    ZeroMemory(&draft, sizeof(draft));
    draft.step_count = context->step_count;
    memcpy(draft.steps, context->steps, sizeof(draft.steps));
    if (!macro_step_remove(&draft, (unsigned int)selected)) {
        editor_set_status(context, L"Não foi possível remover a etapa.");
        return;
    }
    context->step_count = draft.step_count;
    memcpy(context->steps, draft.steps, sizeof(context->steps));
    if (context->step_count == 0) next_index = -1;
    else if ((unsigned int)selected < context->step_count) next_index = (int)selected;
    else next_index = (int)context->step_count - 1;
    editor_refresh_steps(context, next_index);
    if (next_index >= 0) editor_load_selected_step(context);
    else SetWindowTextW(context->step_value, L"");
    editor_set_status(context, L"Etapa removida. Salve a macro para persistir as alterações.");
}

static void editor_move_step(MacroEditorContext *context, int direction) {
    LRESULT selected = SendMessageW(context->steps_list, LB_GETCURSEL, 0, 0);
    int target;
    DeckMacro draft;
    if (selected == LB_ERR || (unsigned long)selected >= context->step_count) {
        editor_set_status(context, L"Selecione uma etapa para reordenar.");
        return;
    }
    target = (int)selected + direction;
    if (target < 0 || (unsigned int)target >= context->step_count) return;
    ZeroMemory(&draft, sizeof(draft));
    draft.step_count = context->step_count;
    memcpy(draft.steps, context->steps, sizeof(draft.steps));
    if (!macro_step_move(&draft, (unsigned int)selected, (unsigned int)target)) return;
    memcpy(context->steps, draft.steps, sizeof(context->steps));
    editor_refresh_steps(context, target);
    editor_load_selected_step(context);
    editor_set_status(context, L"Ordem atualizada. Salve a macro para persistir as alterações.");
}

static void editor_new_macro(MacroEditorContext *context) {
    unsigned int free_slot = DECK_MACRO_COUNT;
    if (!editor_capture_current(context)) return;
    for (unsigned int i = 0; i < DECK_MACRO_COUNT; ++i) {
        if (!context->working.macros[i].configured) {
            free_slot = i;
            break;
        }
    }
    if (free_slot == DECK_MACRO_COUNT) {
        editor_set_status(context, L"Todos os oito espaços de macro estão ocupados.");
        return;
    }
    editor_load_slot(context, free_slot);
    SetFocus(context->name_edit);
}

static void editor_save(HWND hwnd, MacroEditorContext *context) {
    if (!editor_capture_current(context)) return;
    for (unsigned int i = 0; i < DECK_MACRO_COUNT; ++i) {
        if (context->working.macros[i].configured &&
            !macro_validate(&context->working.macros[i])) {
            editor_set_status(context, L"Uma das macros possui uma definição inválida. Revise antes de salvar.");
            return;
        }
    }
    if (!macro_store_save(&context->working)) {
        MessageBoxW(hwnd,
                    L"Não foi possível salvar macros.cfg. O arquivo anterior foi preservado.",
                    L"C-Deck — Multi Action", MB_OK | MB_ICONERROR);
        editor_set_status(context, L"Falha de gravação. Verifique as permissões da pasta config.");
        return;
    }
    *context->destination = context->working;
    context->result = 1;
    DestroyWindow(hwnd);
}

static int editor_create_controls(HWND hwnd, MacroEditorContext *context) {
    DWORD child = WS_CHILD | WS_VISIBLE;
    DWORD edit_style = child | WS_TABSTOP | ES_AUTOHSCROLL;
    DWORD button_style = child | WS_TABSTOP | BS_PUSHBUTTON;

    editor_create_control(hwnd, context, L"STATIC", L"Macro",
                          child | SS_LEFT, 0, 22, 55, 100, 20, 0);
    context->macro_combo = editor_create_control(hwnd, context, L"COMBOBOX", L"",
                          child | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                          0, 22, 77, 260, 220, ID_EDITOR_MACRO_COMBO);
    editor_create_control(hwnd, context, L"STATIC", L"Nome da macro",
                          child | SS_LEFT, 0, 310, 55, 180, 20, 0);
    context->name_edit = editor_create_control(hwnd, context, L"EDIT", L"",
                          edit_style, WS_EX_CLIENTEDGE, 310, 77, 445, 28, ID_EDITOR_NAME);
    SendMessageW(context->name_edit, EM_SETLIMITTEXT, DECK_MACRO_NAME_MAX - 1, 0);

    editor_create_control(hwnd, context, L"STATIC", L"Etapas · use as setas para alterar a ordem",
                          child | SS_LEFT, 0, 22, 125, 430, 20, 0);
    context->steps_list = editor_create_control(hwnd, context, L"LISTBOX", L"",
                          child | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                          LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                          WS_EX_CLIENTEDGE, 22, 148, 340, 305, ID_EDITOR_STEPS);

    editor_create_control(hwnd, context, L"STATIC", L"Tipo da etapa",
                          child | SS_LEFT, 0, 390, 125, 150, 20, 0);
    context->step_type = editor_create_control(hwnd, context, L"COMBOBOX", L"",
                          child | WS_TABSTOP | CBS_DROPDOWNLIST,
                          0, 390, 148, 160, 180, ID_EDITOR_STEP_TYPE);
    SendMessageW(context->step_type, CB_ADDSTRING, 0, (LPARAM)L"Ação interna");
    SendMessageW(context->step_type, CB_ADDSTRING, 0, (LPARAM)L"Abrir destino");
    SendMessageW(context->step_type, CB_ADDSTRING, 0, (LPARAM)L"Aguardar");
    SendMessageW(context->step_type, CB_SETCURSEL, 0, 0);

    editor_create_control(hwnd, context, L"STATIC", L"Valor",
                          child | SS_LEFT, 0, 570, 125, 150, 20, 0);
    context->step_value = editor_create_control(hwnd, context, L"EDIT", L"",
                          edit_style, WS_EX_CLIENTEDGE, 570, 148, 185, 28, ID_EDITOR_STEP_VALUE);
    SendMessageW(context->step_value, EM_SETLIMITTEXT, DECK_MACRO_VALUE_MAX - 1, 0);

    editor_create_control(hwnd, context, L"BUTTON", L"Adicionar etapa",
                          button_style, 0, 390, 194, 160, 34, ID_EDITOR_ADD_STEP);
    editor_create_control(hwnd, context, L"BUTTON", L"Atualizar etapa",
                          button_style, 0, 570, 194, 185, 34, ID_EDITOR_UPDATE_STEP);
    editor_create_control(hwnd, context, L"BUTTON", L"Remover etapa",
                          button_style, 0, 390, 238, 160, 34, ID_EDITOR_REMOVE_STEP);
    editor_create_control(hwnd, context, L"BUTTON", L"↑  Mover acima",
                          button_style, 0, 570, 238, 88, 34, ID_EDITOR_STEP_UP);
    editor_create_control(hwnd, context, L"BUTTON", L"↓  Mover abaixo",
                          button_style, 0, 667, 238, 88, 34, ID_EDITOR_STEP_DOWN);

    editor_create_control(hwnd, context, L"STATIC",
                          L"Ação: volume_up, volume_down, audio_mute, mic_mute, media_play_pause, media_previous, media_next ou sound_devices.\nAbrir: URL HTTP(S), ms-settings:, .exe ou .lnk. Aguardar: 1–5000 ms.",
                          child | SS_LEFT, 0, 390, 292, 365, 100, 0);

    editor_create_control(hwnd, context, L"BUTTON", L"Nova macro",
                          button_style, 0, 22, 474, 120, 34, ID_EDITOR_NEW_MACRO);
    context->status = editor_create_control(hwnd, context, L"STATIC", L"",
                          child | SS_LEFT | SS_CENTERIMAGE, 0, 22, 522, 520, 28, ID_EDITOR_STATUS);
    editor_create_control(hwnd, context, L"BUTTON", L"Salvar alterações",
                          button_style | BS_DEFPUSHBUTTON, 0, 560, 522, 195, 34, ID_EDITOR_SAVE);
    editor_create_control(hwnd, context, L"BUTTON", L"Cancelar",
                          button_style, 0, 560, 562, 195, 28, ID_EDITOR_CANCEL);

    if (!context->macro_combo || !context->name_edit || !context->steps_list ||
        !context->step_type || !context->step_value || !context->status) return 0;
    editor_refresh_macro_slots(context);
    editor_load_slot(context, context->slot);
    return 1;
}

static LRESULT CALLBACK macro_editor_window_proc(HWND hwnd, UINT message,
                                                  WPARAM wparam, LPARAM lparam) {
    MacroEditorContext *context =
        (MacroEditorContext *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (message == WM_NCCREATE) {
        CREATESTRUCTW *create = (CREATESTRUCTW *)lparam;
        context = (MacroEditorContext *)create->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)context);
    }

    switch (message) {
        case WM_CREATE:
            if (!context || !editor_create_controls(hwnd, context)) {
                if (context) context->result = -1;
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO *limits = (MINMAXINFO *)lparam;
            limits->ptMinTrackSize.x = EDITOR_WIDTH;
            limits->ptMinTrackSize.y = EDITOR_HEIGHT;
            return 0;
        }

        case WM_ERASEBKGND: {
            RECT client;
            GetClientRect(hwnd, &client);
            FillRect((HDC)wparam, &client, context ? context->background : (HBRUSH)(COLOR_WINDOW + 1));
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT paint;
            RECT client;
            HDC dc = BeginPaint(hwnd, &paint);
            GetClientRect(hwnd, &client);
            FillRect(dc, &client, context ? context->background : (HBRUSH)(COLOR_WINDOW + 1));
            if (context && context->font) {
                HGDIOBJ previous = SelectObject(dc, context->font);
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, RGB(225, 229, 235));
                RECT heading = { 22, 18, client.right - 22, 42 };
                DrawTextW(dc, L"C-DECK  /  EDITOR DE MULTI ACTION", -1, &heading,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                SelectObject(dc, previous);
            }
            EndPaint(hwnd, &paint);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(190, 198, 209));
            SetBkMode(dc, TRANSPARENT);
            return (LRESULT)(context ? context->background : GetStockObject(BLACK_BRUSH));
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(238, 241, 245));
            SetBkColor(dc, RGB(25, 28, 34));
            return (LRESULT)(context ? context->input_background : GetStockObject(BLACK_BRUSH));
        }

        case WM_CTLCOLORBTN: {
            HDC dc = (HDC)wparam;
            SetTextColor(dc, RGB(225, 229, 235));
            SetBkColor(dc, RGB(31, 35, 42));
            return (LRESULT)(context ? context->background : GetStockObject(BLACK_BRUSH));
        }

        case WM_COMMAND: {
            int id = LOWORD(wparam);
            int notification = HIWORD(wparam);
            if (!context) break;

            if (id == ID_EDITOR_MACRO_COMBO && notification == CBN_SELCHANGE) {
                LRESULT selected = SendMessageW(context->macro_combo, CB_GETCURSEL, 0, 0);
                if (selected != CB_ERR && (unsigned int)selected < DECK_MACRO_COUNT) {
                    unsigned int previous = context->slot;
                    if (!editor_capture_current(context)) {
                        SendMessageW(context->macro_combo, CB_SETCURSEL, previous, 0);
                    } else {
                        editor_load_slot(context, (unsigned int)selected);
                    }
                }
                return 0;
            }
            if (id == ID_EDITOR_STEPS && notification == LBN_SELCHANGE) {
                editor_load_selected_step(context);
                return 0;
            }
            if (notification != BN_CLICKED) break;
            if (id == ID_EDITOR_ADD_STEP) editor_add_step(context);
            else if (id == ID_EDITOR_UPDATE_STEP) editor_update_step(context);
            else if (id == ID_EDITOR_REMOVE_STEP) editor_remove_step(context);
            else if (id == ID_EDITOR_STEP_UP) editor_move_step(context, -1);
            else if (id == ID_EDITOR_STEP_DOWN) editor_move_step(context, 1);
            else if (id == ID_EDITOR_NEW_MACRO) editor_new_macro(context);
            else if (id == ID_EDITOR_SAVE) editor_save(hwnd, context);
            else if (id == ID_EDITOR_CANCEL) DestroyWindow(hwnd);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

static int editor_register_class(HINSTANCE instance) {
    WNDCLASSEXW window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = macro_editor_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    window_class.lpszClassName = EDITOR_CLASS_NAME;
    if (RegisterClassExW(&window_class)) return 1;
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

int macro_editor_show(HWND parent, DeckMacroStore *store) {
    MacroEditorContext context;
    HINSTANCE instance = GetModuleHandleW(NULL);
    HWND window;
    RECT parent_rect;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    MSG message;

    if (!store || !store->loaded || store->read_only) return -1;
    if (!editor_register_class(instance)) return -1;

    ZeroMemory(&context, sizeof(context));
    context.working = *store;
    context.destination = store;
    context.result = 0;
    context.slot = 0;
    for (unsigned int i = 0; i < DECK_MACRO_COUNT; ++i) {
        if (context.working.macros[i].configured) {
            context.slot = i;
            break;
        }
    }
    context.font = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    context.background = CreateSolidBrush(RGB(20, 23, 28));
    context.input_background = CreateSolidBrush(RGB(25, 28, 34));

    if (parent && GetWindowRect(parent, &parent_rect)) {
        x = parent_rect.left + ((parent_rect.right - parent_rect.left) - EDITOR_WIDTH) / 2;
        y = parent_rect.top + ((parent_rect.bottom - parent_rect.top) - EDITOR_HEIGHT) / 2;
    }

    window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        EDITOR_CLASS_NAME, L"C-Deck — Editor de Multi Action",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        x, y, EDITOR_WIDTH, EDITOR_HEIGHT,
        parent, NULL, instance, &context);
    if (!window) {
        if (context.font) DeleteObject(context.font);
        if (context.background) DeleteObject(context.background);
        if (context.input_background) DeleteObject(context.input_background);
        return -1;
    }

    if (parent) EnableWindow(parent, FALSE);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetForegroundWindow(window);

    while (IsWindow(window)) {
        int received = GetMessageW(&message, NULL, 0, 0);
        if (received <= 0) {
            if (received == 0) PostQuitMessage((int)message.wParam);
            break;
        }
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    /* WM_QUIT and GetMessageW failures can end the modal loop without WM_CLOSE.
       Destroy the window while its stack-backed context and GDI resources are
       still alive; otherwise the editor becomes an orphan with dangling state. */
    if (IsWindow(window)) {
        DestroyWindow(window);
    }

    if (parent && IsWindow(parent)) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }
    if (context.font) DeleteObject(context.font);
    if (context.background) DeleteObject(context.background);
    if (context.input_background) DeleteObject(context.input_background);
    return context.result;
}
