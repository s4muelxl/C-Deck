#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "macro_editor.h"

#define EDITOR_CLASS_NAME L"CDeckMacroEditorWindow"
#define ID_EDITOR_MACRO_COMBO 6101
#define ID_EDITOR_NAME 6102
#define ID_EDITOR_STEPS 6103
#define ID_EDITOR_STEP_TYPE 6104
#define ID_EDITOR_STEP_VALUE 6105
#define ID_EDITOR_ADD_STEP 6106
#define ID_EDITOR_UPDATE_STEP 6107
#define ID_EDITOR_REMOVE_STEP 6108
#define ID_EDITOR_STEP_UP 6109
#define ID_EDITOR_NEW_MACRO 6111
#define ID_EDITOR_SAVE 6112
#define ID_EDITOR_CANCEL 6113

typedef enum EditorScenario {
    EDITOR_SCENARIO_CREATE_AND_SAVE,
    EDITOR_SCENARIO_CREATE_LONG_OPEN_AND_SAVE,
    EDITOR_SCENARIO_EDIT_REORDER_REMOVE_AND_SAVE,
    EDITOR_SCENARIO_SWITCH_SLOT_PRESERVES_DRAFT,
    EDITOR_SCENARIO_EDIT_AND_CANCEL
} EditorScenario;

typedef struct EditorDriver {
    DWORD ui_thread_id;
    EditorScenario scenario;
    volatile LONG window_found;
    volatile LONG completed;
    volatile LONG failed;
    volatile LONG failure_stage;
} EditorDriver;

static HWND wait_for_editor(void) {
    for (unsigned int attempt = 0; attempt < 500; ++attempt) {
        HWND editor = FindWindowW(EDITOR_CLASS_NAME, NULL);
        if (editor && IsWindowVisible(editor) &&
            GetDlgItem(editor, ID_EDITOR_NAME) &&
            GetDlgItem(editor, ID_EDITOR_STEP_VALUE) &&
            GetDlgItem(editor, ID_EDITOR_STEPS)) {
            return editor;
        }
        Sleep(10);
    }
    return NULL;
}

static int click_control(HWND editor, int control_id) {
    HWND control = GetDlgItem(editor, control_id);
    if (!control) return 0;
    SendMessageW(control, BM_CLICK, 0, 0);
    return 1;
}

static DWORD WINAPI drive_editor(LPVOID parameter) {
    EditorDriver *driver = (EditorDriver *)parameter;
    HWND editor = wait_for_editor();
    HWND name_edit;
    HWND step_value;
    HWND steps_list;

    if (!editor) {
        InterlockedExchange(&driver->failure_stage, 1);
        InterlockedExchange(&driver->failed, 1);
        PostThreadMessageW(driver->ui_thread_id, WM_QUIT, 91, 0);
        InterlockedExchange(&driver->completed, 1);
        return 0;
    }

    InterlockedExchange(&driver->window_found, 1);
    name_edit = GetDlgItem(editor, ID_EDITOR_NAME);
    step_value = GetDlgItem(editor, ID_EDITOR_STEP_VALUE);
    steps_list = GetDlgItem(editor, ID_EDITOR_STEPS);
    if (!name_edit || !step_value || !steps_list) {
        InterlockedExchange(&driver->failure_stage, 2);
        InterlockedExchange(&driver->failed, 1);
        PostMessageW(editor, WM_CLOSE, 0, 0);
        InterlockedExchange(&driver->completed, 1);
        return 0;
    }

    if (driver->scenario == EDITOR_SCENARIO_CREATE_AND_SAVE) {
        if (!click_control(editor, ID_EDITOR_NEW_MACRO)) {
            InterlockedExchange(&driver->failure_stage, 3);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SetWindowTextW(name_edit, L"Macro criada pelo teste");
        SetWindowTextW(step_value, L"volume_up");
        if (!click_control(editor, ID_EDITOR_ADD_STEP) ||
            SendMessageW(steps_list, LB_GETCOUNT, 0, 0) != 1) {
            InterlockedExchange(&driver->failure_stage, 4);
            InterlockedExchange(&driver->failed, 1);
            click_control(editor, ID_EDITOR_CANCEL);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        if (!click_control(editor, ID_EDITOR_SAVE)) {
            InterlockedExchange(&driver->failure_stage, 5);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
        }
    } else if (driver->scenario == EDITOR_SCENARIO_CREATE_LONG_OPEN_AND_SAVE) {
        wchar_t long_target[512];
        const wchar_t prefix[] = L"C:\\Aplicações\\Δ\\";
        size_t prefix_length = wcslen(prefix);
        HWND step_type = GetDlgItem(editor, ID_EDITOR_STEP_TYPE);

        if (!step_type || !click_control(editor, ID_EDITOR_NEW_MACRO)) {
            InterlockedExchange(&driver->failure_stage, 7);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }
        memcpy(long_target, prefix, sizeof(prefix));
        for (size_t i = 0; i < 400; ++i) long_target[prefix_length + i] = L'a';
        wcscpy(long_target + prefix_length + 400, L".exe");
        SetWindowTextW(name_edit, L"Macro destino longo");
        SendMessageW(step_type, CB_SETCURSEL, 1, 0);
        SetWindowTextW(step_value, long_target);
        if (!click_control(editor, ID_EDITOR_ADD_STEP) ||
            SendMessageW(steps_list, LB_GETCOUNT, 0, 0) != 1) {
            InterlockedExchange(&driver->failure_stage, 8);
            InterlockedExchange(&driver->failed, 1);
            click_control(editor, ID_EDITOR_CANCEL);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }
        if (!click_control(editor, ID_EDITOR_SAVE)) {
            InterlockedExchange(&driver->failure_stage, 9);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
        }
    } else if (driver->scenario == EDITOR_SCENARIO_EDIT_REORDER_REMOVE_AND_SAVE) {
        HWND step_type = GetDlgItem(editor, ID_EDITOR_STEP_TYPE);
        if (!step_type || SendMessageW(steps_list, LB_GETCOUNT, 0, 0) != 3) {
            InterlockedExchange(&driver->failure_stage, 10);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        /* Move the third step above the second, then replace and remove through Win32 controls. */
        SendMessageW(steps_list, LB_SETCURSEL, 2, 0);
        if (!click_control(editor, ID_EDITOR_STEP_UP) ||
            SendMessageW(steps_list, LB_GETCOUNT, 0, 0) != 3) {
            InterlockedExchange(&driver->failure_stage, 11);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SendMessageW(steps_list, LB_SETCURSEL, 1, 0);
        SendMessageW(step_type, CB_SETCURSEL, 0, 0);
        SetWindowTextW(step_value, L"media_previous");
        if (!click_control(editor, ID_EDITOR_UPDATE_STEP)) {
            InterlockedExchange(&driver->failure_stage, 12);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SendMessageW(steps_list, LB_SETCURSEL, 0, 0);
        if (!click_control(editor, ID_EDITOR_REMOVE_STEP) ||
            SendMessageW(steps_list, LB_GETCOUNT, 0, 0) != 2) {
            InterlockedExchange(&driver->failure_stage, 13);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SetWindowTextW(name_edit, L"Macro editada pelo teste");
        if (!click_control(editor, ID_EDITOR_SAVE)) {
            InterlockedExchange(&driver->failure_stage, 14);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
        }
    } else if (driver->scenario == EDITOR_SCENARIO_SWITCH_SLOT_PRESERVES_DRAFT) {
        HWND combo = GetDlgItem(editor, ID_EDITOR_MACRO_COMBO);
        wchar_t current_name[DECK_MACRO_NAME_MAX * 2];
        if (!combo) {
            InterlockedExchange(&driver->failure_stage, 15);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SetWindowTextW(name_edit, L"Rascunho antes da troca");
        SendMessageW(combo, CB_SETCURSEL, 1, 0);
        SendMessageW(editor, WM_COMMAND,
                     MAKEWPARAM(ID_EDITOR_MACRO_COMBO, CBN_SELCHANGE), (LPARAM)combo);
        GetWindowTextW(name_edit, current_name,
                       (int)(sizeof(current_name) / sizeof(current_name[0])));
        if (wcscmp(current_name, L"Macro destino longo") != 0) {
            InterlockedExchange(&driver->failure_stage, 16);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
            InterlockedExchange(&driver->completed, 1);
            return 0;
        }

        SetWindowTextW(name_edit, L"Macro longa preservada");
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
        SendMessageW(editor, WM_COMMAND,
                     MAKEWPARAM(ID_EDITOR_MACRO_COMBO, CBN_SELCHANGE), (LPARAM)combo);
        GetWindowTextW(name_edit, current_name,
                       (int)(sizeof(current_name) / sizeof(current_name[0])));
        if (wcscmp(current_name, L"Rascunho antes da troca") != 0 ||
            !click_control(editor, ID_EDITOR_SAVE)) {
            InterlockedExchange(&driver->failure_stage, 17);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
        }
    } else {
        SetWindowTextW(name_edit, L"Alteração que deve ser descartada");
        if (!click_control(editor, ID_EDITOR_CANCEL)) {
            InterlockedExchange(&driver->failure_stage, 6);
            InterlockedExchange(&driver->failed, 1);
            PostMessageW(editor, WM_CLOSE, 0, 0);
        }
    }

    InterlockedExchange(&driver->completed, 1);
    return 0;
}

static void remove_store_files(const wchar_t *directory) {
    wchar_t path[DECK_MACRO_PATH_MAX];
    swprintf(path, DECK_MACRO_PATH_MAX, L"%ls\\macros.cfg.tmp", directory);
    DeleteFileW(path);
    swprintf(path, DECK_MACRO_PATH_MAX, L"%ls\\macros.cfg.bak", directory);
    DeleteFileW(path);
    swprintf(path, DECK_MACRO_PATH_MAX, L"%ls\\macros.cfg", directory);
    DeleteFileW(path);
}

static int run_editor_scenario(HWND parent, DeckMacroStore *store,
                               EditorScenario scenario, int expected_result) {
    EditorDriver driver;
    HANDLE thread;
    int result;

    ZeroMemory(&driver, sizeof(driver));
    driver.ui_thread_id = GetCurrentThreadId();
    driver.scenario = scenario;

    thread = CreateThread(NULL, 0, drive_editor, &driver, 0, NULL);
    assert(thread != NULL);

    result = macro_editor_show(parent, store);

    assert(WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0);
    CloseHandle(thread);
    assert(driver.completed == 1);
    assert(driver.window_found == 1);
    if (driver.failed) {
        fprintf(stderr, "macro editor workflow driver failed at stage %ld\n",
                (long)driver.failure_stage);
    }
    assert(driver.failed == 0);
    assert(result == expected_result);
    assert(IsWindow(parent));
    assert(IsWindowEnabled(parent));
    assert(FindWindowW(EDITOR_CLASS_NAME, NULL) == NULL);
    return result;
}

int main(void) {
    const char *config_path = "macro-editor-workflow-Δ/apps.cfg";
    const char *directory_utf8 = "macro-editor-workflow-Δ";
    wchar_t directory[DECK_MACRO_PATH_MAX];
    wchar_t persisted_path[DECK_MACRO_PATH_MAX];
    static DeckMacroStore store;
    static DeckMacroStore reloaded;
    static DeckMacroStore before_cancel;
    HWND parent;

    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                               directory_utf8, -1,
                               directory, DECK_MACRO_PATH_MAX) > 0);
    assert(CreateDirectoryW(directory, NULL) ||
           GetLastError() == ERROR_ALREADY_EXISTS);
    remove_store_files(directory);

    assert(macro_store_load(&store, config_path));
    assert(store.loaded && !store.read_only);
    parent = CreateWindowExW(0, L"STATIC", L"Macro editor workflow test",
                             WS_POPUP, 0, 0, 240, 120,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    assert(parent != NULL);
    assert(IsWindowEnabled(parent));

    run_editor_scenario(parent, &store, EDITOR_SCENARIO_CREATE_AND_SAVE, 1);
    assert(macro_store_count(&store) == 1);
    assert(macro_store_get(&store, 1) != NULL);
    assert(macro_validate(macro_store_get(&store, 1)));
    assert(strcmp(macro_store_get(&store, 1)->name,
                  "Macro criada pelo teste") == 0);
    assert(macro_store_get(&store, 1)->step_count == 1);
    assert(strcmp(macro_store_get(&store, 1)->steps[0].value,
                  "volume_up") == 0);

    run_editor_scenario(parent, &store,
                        EDITOR_SCENARIO_CREATE_LONG_OPEN_AND_SAVE, 1);
    assert(macro_store_count(&store) == 2);
    {
        const DeckMacro *long_macro = macro_store_get(&store, 2);
        assert(long_macro != NULL && long_macro->step_count == 1);
        assert(long_macro->steps[0].type == DECK_MACRO_STEP_OPEN);
        assert(strlen(long_macro->steps[0].value) > 260);
        assert(strstr(long_macro->steps[0].value, "Aplicações") != NULL);
        assert(strstr(long_macro->steps[0].value, ".exe") != NULL);
    }

    /* Seed three valid steps so the native editor's update/remove/reorder controls
       are exercised as a complete persisted workflow, not only as model helpers. */
    {
        DeckMacroStep step;
        DeckMacro *first_macro = &store.macros[0];
        assert(macro_step_parse("action:media_next", &step));
        assert(macro_step_insert(first_macro, 1, &step));
        assert(macro_step_parse("delay:250", &step));
        assert(macro_step_insert(first_macro, 2, &step));
        assert(macro_store_save(&store));
    }

    assert(macro_store_load(&reloaded, config_path));
    assert(macro_store_count(&reloaded) == 2);
    assert(strlen(macro_store_get(&reloaded, 2)->steps[0].value) > 260);
    assert(strcmp(macro_store_get(&reloaded, 1)->name,
                  "Macro criada pelo teste") == 0);

    run_editor_scenario(parent, &reloaded,
                        EDITOR_SCENARIO_EDIT_REORDER_REMOVE_AND_SAVE, 1);
    {
        const DeckMacro *edited = macro_store_get(&reloaded, 1);
        assert(edited != NULL && edited->step_count == 2);
        assert(strcmp(edited->name, "Macro editada pelo teste") == 0);
        assert(edited->steps[0].type == DECK_MACRO_STEP_ACTION);
        assert(strcmp(edited->steps[0].value, "media_previous") == 0);
        assert(edited->steps[1].type == DECK_MACRO_STEP_ACTION);
        assert(strcmp(edited->steps[1].value, "media_next") == 0);
    }

    run_editor_scenario(parent, &reloaded,
                        EDITOR_SCENARIO_SWITCH_SLOT_PRESERVES_DRAFT, 1);
    assert(strcmp(macro_store_get(&reloaded, 1)->name,
                  "Rascunho antes da troca") == 0);
    assert(strcmp(macro_store_get(&reloaded, 2)->name,
                  "Macro longa preservada") == 0);

    before_cancel = reloaded;
    run_editor_scenario(parent, &reloaded, EDITOR_SCENARIO_EDIT_AND_CANCEL, 0);
    assert(memcmp(&reloaded, &before_cancel, sizeof(reloaded)) == 0);

    assert(macro_store_load(&store, config_path));
    assert(strcmp(macro_store_get(&store, 1)->name,
                  "Rascunho antes da troca") == 0);
    assert(strcmp(macro_store_get(&store, 2)->name,
                  "Macro longa preservada") == 0);
    assert(macro_store_get(&store, 1)->step_count == 2);
    assert(strcmp(macro_store_get(&store, 1)->steps[0].value,
                  "media_previous") == 0);

    assert(DestroyWindow(parent));
    remove_store_files(directory);
    swprintf(persisted_path, DECK_MACRO_PATH_MAX, L"%ls", directory);
    assert(RemoveDirectoryW(persisted_path));

    puts("macro editor create, update, remove, reorder, slot switch, save, reload and cancel: OK");
    return 0;
}
