#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "macro_completion_gate.h"
#include <assert.h>
#include <stdio.h>

#define TEST_COMPLETION_MESSAGE (WM_APP + 77)
static volatile LONG g_received;

static LRESULT CALLBACK test_window_proc(HWND hwnd, UINT message,
                                         WPARAM wparam, LPARAM lparam) {
    if (message == TEST_COMPLETION_MESSAGE) {
        InterlockedIncrement(&g_received);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

typedef struct DelayedPostArgs {
    DeckMacroCompletionGate *gate;
    HANDLE start_event;
    BOOL posted;
} DelayedPostArgs;

static DWORD WINAPI delayed_post_thread(LPVOID parameter) {
    DelayedPostArgs *args = (DelayedPostArgs *)parameter;
    if (!args) return 1;
    if (WaitForSingleObject(args->start_event, INFINITE) != WAIT_OBJECT_0) return 2;
    args->posted = deck_macro_completion_gate_post(args->gate,
                                                   TEST_COMPLETION_MESSAGE, 3, 4);
    return 0;
}

static void dispatch_test_messages(HWND hwnd) {
    MSG message;
    while (PeekMessageW(&message, hwnd, TEST_COMPLETION_MESSAGE,
                        TEST_COMPLETION_MESSAGE, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

int main(void) {
    static const wchar_t class_name[] = L"CDeckMacroCompletionGateTest";
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSW window_class;
    HWND hwnd;
    DeckMacroCompletionGate gate = DECK_MACRO_COMPLETION_GATE_INITIALIZER;
    HANDLE start_event;
    HANDLE worker;
    DelayedPostArgs args;

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = test_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    if (!RegisterClassW(&window_class)) assert(GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
    hwnd = CreateWindowExW(0, class_name, L"", 0, 0, 0, 0, 0,
                           HWND_MESSAGE, NULL, instance, NULL);
    assert(hwnd != NULL);

    deck_macro_completion_gate_bind(&gate, hwnd);
    assert(deck_macro_completion_gate_post(&gate, TEST_COMPLETION_MESSAGE, 1, 2));
    dispatch_test_messages(hwnd);
    assert(InterlockedCompareExchange(&g_received, 0, 0) == 1);

    start_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    assert(start_event != NULL);
    args.gate = &gate;
    args.start_event = start_event;
    args.posted = TRUE;
    worker = CreateThread(NULL, 0, delayed_post_thread, &args, 0, NULL);
    assert(worker != NULL);

    /* Closing first must make a worker that wakes later unable to post. */
    deck_macro_completion_gate_close(&gate);
    assert(!deck_macro_completion_gate_post(&gate, TEST_COMPLETION_MESSAGE, 5, 6));
    assert(SetEvent(start_event));
    assert(WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0);
    assert(!args.posted);
    dispatch_test_messages(hwnd);
    assert(InterlockedCompareExchange(&g_received, 0, 0) == 1);

    /* Rebinding does not reinitialize the SRWLOCK. */
    deck_macro_completion_gate_bind(&gate, hwnd);
    assert(deck_macro_completion_gate_post(&gate, TEST_COMPLETION_MESSAGE, 7, 8));
    dispatch_test_messages(hwnd);
    assert(InterlockedCompareExchange(&g_received, 0, 0) == 2);
    deck_macro_completion_gate_close(&gate);

    CloseHandle(worker);
    CloseHandle(start_event);
    DestroyWindow(hwnd);
    UnregisterClassW(class_name, instance);
    puts("macro completion gate shutdown and rebind: OK");
    return 0;
}
