#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "macro_editor.h"

#define EDITOR_TEST_CLASS L"CDeckMacroEditorWindow"
#define TEST_QUIT_CODE 23

typedef struct QuitRequest {
    DWORD ui_thread_id;
    volatile LONG window_found;
    volatile LONG quit_posted;
} QuitRequest;

static DWORD WINAPI post_editor_quit(LPVOID parameter) {
    QuitRequest *request = (QuitRequest *)parameter;
    HWND editor = NULL;

    for (unsigned int attempt = 0; attempt < 500; ++attempt) {
        editor = FindWindowW(EDITOR_TEST_CLASS, NULL);
        if (editor) break;
        Sleep(10);
    }

    if (editor) InterlockedExchange(&request->window_found, 1);

    if (PostThreadMessageW(request->ui_thread_id, WM_QUIT, TEST_QUIT_CODE, 0)) {
        InterlockedExchange(&request->quit_posted, 1);
    } else if (editor) {
        /* Keep the test bounded if the UI thread queue is unexpectedly unavailable. */
        PostMessageW(editor, WM_CLOSE, 0, 0);
    }

    return 0;
}

int main(void) {
    DeckMacroStore store;
    QuitRequest request;
    HWND parent;
    HANDLE quit_thread;
    DWORD thread_result = 1;
    int editor_result;
    MSG quit_message;

    ZeroMemory(&store, sizeof(store));
    store.loaded = 1;
    ZeroMemory(&request, sizeof(request));
    request.ui_thread_id = GetCurrentThreadId();

    parent = CreateWindowExW(0, L"STATIC", L"Macro editor lifecycle test",
                             WS_POPUP, 0, 0, 240, 120,
                             NULL, NULL, GetModuleHandleW(NULL), NULL);
    assert(parent != NULL);
    assert(IsWindowEnabled(parent));

    quit_thread = CreateThread(NULL, 0, post_editor_quit, &request, 0, NULL);
    assert(quit_thread != NULL);

    editor_result = macro_editor_show(parent, &store);

    assert(WaitForSingleObject(quit_thread, 10000) == WAIT_OBJECT_0);
    assert(GetExitCodeThread(quit_thread, &thread_result));
    CloseHandle(quit_thread);

    assert(thread_result == 0);
    assert(request.window_found == 1);
    assert(request.quit_posted == 1);
    assert(editor_result == 0);
    assert(IsWindow(parent));
    assert(IsWindowEnabled(parent));
    assert(FindWindowW(EDITOR_TEST_CLASS, NULL) == NULL);

    /* The modal editor must preserve the application's quit request. */
    assert(PeekMessageW(&quit_message, NULL, WM_QUIT, WM_QUIT, PM_REMOVE));
    assert(quit_message.wParam == TEST_QUIT_CODE);

    assert(DestroyWindow(parent));
    puts("macro editor modal teardown: OK");
    return 0;
}
