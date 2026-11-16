#include "../src/macros.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>

static int action_calls;
static int open_calls;
static int delay_calls;
static int fail_action;
static int cancel_now;
static int cancel_on_action;
static int cancel_on_open;
static int cancel_on_delay;
static char last_open_target[DECK_MACRO_VALUE_MAX];

static int test_action(const char *action_id, void *context) {
    (void)context;
    assert(action_id && action_id[0]);
    ++action_calls;
    if (cancel_on_action) cancel_now = 1;
    return fail_action;
}

static int test_open(const char *target, void *context) {
    (void)context;
    assert(target && target[0]);
    assert(strlen(target) < sizeof(last_open_target));
    strcpy(last_open_target, target);
    ++open_calls;
    if (cancel_on_open) cancel_now = 1;
    return 0;
}

static int test_delay(unsigned int milliseconds, void *context) {
    (void)context;
    assert(milliseconds > 0 && milliseconds <= DECK_MACRO_STEP_DELAY_MAX_MS);
    ++delay_calls;
    if (cancel_on_delay) cancel_now = 1;
    return 0;
}

static int test_cancel(void *context) {
    (void)context;
    return cancel_now;
}

static int file_contains(const char *path, const char *needle) {
    FILE *file = fopen(path, "rb");
    char buffer[DECK_MACRO_LINE_MAX + 1];
    size_t length;
    int found;
    if (!file) return 0;
    length = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[length] = '\0';
    found = strstr(buffer, needle) != NULL;
    fclose(file);
    return found;
}

static void write_text(const char *path, const char *content) {
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    assert(fputs(content, file) >= 0);
    assert(fclose(file) == 0);
}

static void assert_invalid_store_is_read_only(const char *contents, const char *marker) {
    static DeckMacroStore invalid;
    write_text("macros.cfg", contents);
    assert(!macro_store_load(&invalid, "macros-test-apps.cfg"));
    assert(invalid.read_only);
    assert(!macro_store_save(&invalid));
    assert(file_contains("macros.cfg", marker));
}

static void build_valid_macro(DeckMacro *macro) {
    memset(macro, 0, sizeof(*macro));
    strcpy(macro->name, "Preparar transmissão – teste");
    macro->configured = 1;
    macro->step_count = 3;
    macro->steps[0].type = DECK_MACRO_STEP_ACTION;
    strcpy(macro->steps[0].value, "volume_down");
    macro->steps[1].type = DECK_MACRO_STEP_OPEN;
    strcpy(macro->steps[1].value, "C:\\Program Files\\OBS Studio\\bin\\应用.exe");
    macro->steps[2].type = DECK_MACRO_STEP_DELAY;
    macro->steps[2].delay_ms = 500;
}


static void test_missing_macro_parent_is_not_treated_as_empty(void) {
    wchar_t temp_directory[MAX_PATH];
    wchar_t temporary_file[MAX_PATH];
    wchar_t missing_parent[DECK_PATH_WIDE_MAX];
    wchar_t missing_config[DECK_PATH_WIDE_MAX];
    wchar_t missing_macro[DECK_PATH_WIDE_MAX];
    char config_path[DECK_MACRO_PATH_MAX];
    static DeckMacroStore missing_store;
    DWORD temp_length;
    int required;

    temp_length = GetTempPathW(MAX_PATH, temp_directory);
    assert(temp_length > 0 && temp_length < MAX_PATH);
    assert(GetTempFileNameW(temp_directory, L"cdk", 0, temporary_file) != 0);
    assert(DeleteFileW(temporary_file));

    assert(swprintf(missing_parent, DECK_PATH_WIDE_MAX,
                    L"%ls-missing-parent-配置-Δ", temporary_file) > 0);
    SetLastError(ERROR_SUCCESS);
    assert(GetFileAttributesW(missing_parent) == INVALID_FILE_ATTRIBUTES);
    assert(GetLastError() == ERROR_FILE_NOT_FOUND);

    assert(swprintf(missing_config, DECK_PATH_WIDE_MAX,
                    L"%ls\\apps.cfg", missing_parent) > 0);
    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                   missing_config, -1, NULL, 0, NULL, NULL);
    assert(required > 0 && (size_t)required <= sizeof(config_path));
    assert(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                               missing_config, -1, config_path, required,
                               NULL, NULL) == required);

    /* A missing file in an existing directory is a valid empty macro store.
       A missing parent directory is a storage error and must fail closed. */
    assert(!macro_store_load(&missing_store, config_path));
    assert(!missing_store.loaded);
    assert(missing_store.read_only);
    assert(!macro_store_save(&missing_store));

    assert(swprintf(missing_macro, DECK_PATH_WIDE_MAX,
                    L"%ls\\macros.cfg", missing_parent) > 0);
    SetLastError(ERROR_SUCCESS);
    assert(GetFileAttributesW(missing_macro) == INVALID_FILE_ATTRIBUTES);
    assert(GetLastError() == ERROR_PATH_NOT_FOUND);
}

int main(void) {
    DeckMacroStep step;
    test_missing_macro_parent_is_not_treated_as_empty();
    DeckMacro macro;
    static DeckMacroStore store;
    static DeckMacroStore loaded;
    DeckMacro editing;

    assert(macro_step_parse("action:volume_up", &step));
    assert(step.type == DECK_MACRO_STEP_ACTION);
    assert(strcmp(step.value, "volume_up") == 0);
    assert(macro_step_parse("open:https://example.com/live", &step));
    assert(step.type == DECK_MACRO_STEP_OPEN);
    assert(macro_step_parse("open:https://例え.テスト/路径", &step));
    assert(strcmp(step.value, "https://例え.テスト/路径") == 0);
    assert(macro_step_parse("open:C:\\Program Files\\OBS Studio\\bin\\应用.exe", &step));
    assert(strcmp(step.value, "C:\\Program Files\\OBS Studio\\bin\\应用.exe") == 0);
    assert(!macro_step_parse("open:C:\\Program Files\\OBS Studio\\bin\\应用.exe --profile", &step));
    assert(!macro_step_parse("open:\"C:\\Program Files\\OBS Studio\\bin\\应用.exe\"", &step));
    assert(macro_step_parse("delay:250", &step));
    assert(step.type == DECK_MACRO_STEP_DELAY && step.delay_ms == 250);
    assert(!macro_step_parse("action:run_shell", &step));
    assert(!macro_step_parse("open:powershell.exe -Command whoami", &step));
    assert(!macro_step_parse("open:javascript:alert(1)", &step));
    assert(!macro_step_parse("delay:0", &step));
    assert(!macro_step_parse("delay:5001", &step));

    build_valid_macro(&editing);
    assert(macro_step_parse("action:media_next", &step));
    assert(macro_step_insert(&editing, 1, &step));
    assert(editing.step_count == 4);
    assert(strcmp(editing.steps[1].value, "media_next") == 0);
    assert(macro_step_move(&editing, 1, 3));
    assert(strcmp(editing.steps[3].value, "media_next") == 0);
    assert(macro_step_parse("action:media_previous", &step));
    assert(macro_step_replace(&editing, 3, &step));
    assert(strcmp(editing.steps[3].value, "media_previous") == 0);
    assert(macro_step_remove(&editing, 3));
    assert(editing.step_count == 3);
    assert(!macro_step_remove(&editing, 3));
    assert(!macro_step_move(&editing, 0, 3));
    assert(!macro_step_insert(&editing, DECK_MACRO_STEP_COUNT + 1, &step));
    {
        DeckMacroStep invalid_step;
        memset(&invalid_step, 0, sizeof(invalid_step));
        invalid_step.type = DECK_MACRO_STEP_ACTION;
        strcpy(invalid_step.value, "run_shell");
        assert(!macro_step_insert(&editing, 0, &invalid_step));
    }

    build_valid_macro(&macro);
    assert(macro_validate(&macro));

    /* Fixed-width records without an in-bounds NUL must fail closed. */
    {
        DeckMacro malformed;
        build_valid_macro(&malformed);
        memset(malformed.name, 'N', sizeof(malformed.name));
        assert(!macro_validate(&malformed));
        action_calls = open_calls = delay_calls = 0;
        assert(macro_execute(&malformed, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_INVALID_DEFINITION);
        assert(action_calls == 0 && open_calls == 0 && delay_calls == 0);

        build_valid_macro(&malformed);
        memset(malformed.steps[0].value, 'A', sizeof(malformed.steps[0].value));
        assert(!macro_validate(&malformed));
        assert(macro_execute(&malformed, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_INVALID_DEFINITION);
        assert(action_calls == 0 && open_calls == 0 && delay_calls == 0);

        build_valid_macro(&malformed);
        memset(malformed.steps[1].value, 'B', sizeof(malformed.steps[1].value));
        assert(!macro_validate(&malformed));
        assert(macro_execute(&malformed, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_INVALID_DEFINITION);
        assert(action_calls == 0 && open_calls == 0 && delay_calls == 0);
    }

    action_calls = open_calls = delay_calls = 0;
    fail_action = 0;
    cancel_now = 0;
    assert(macro_execute(&macro, test_action, test_open, test_delay, test_cancel, NULL) == DECK_MACRO_OK);
    assert(action_calls == 1 && open_calls == 1 && delay_calls == 1);
    assert(strcmp(last_open_target, "C:\\Program Files\\OBS Studio\\bin\\应用.exe") == 0);

    /* Cancellation raised by the final executor must never be reported as success. */
    {
        DeckMacro cancel_macro;
        memset(&cancel_macro, 0, sizeof(cancel_macro));
        strcpy(cancel_macro.name, "Cancelamento durante ação");
        cancel_macro.configured = 1;
        cancel_macro.step_count = 1;
        cancel_macro.steps[0].type = DECK_MACRO_STEP_ACTION;
        strcpy(cancel_macro.steps[0].value, "volume_up");

        action_calls = open_calls = delay_calls = 0;
        fail_action = cancel_now = 0;
        cancel_on_action = 1;
        cancel_on_open = cancel_on_delay = 0;
        assert(macro_execute(&cancel_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_CANCELLED);
        assert(action_calls == 1 && open_calls == 0 && delay_calls == 0);

        memset(&cancel_macro, 0, sizeof(cancel_macro));
        strcpy(cancel_macro.name, "Cancelamento durante abertura");
        cancel_macro.configured = 1;
        cancel_macro.step_count = 1;
        cancel_macro.steps[0].type = DECK_MACRO_STEP_OPEN;
        strcpy(cancel_macro.steps[0].value, "https://example.com/");
        action_calls = open_calls = delay_calls = 0;
        cancel_now = 0;
        cancel_on_action = 0;
        cancel_on_open = 1;
        assert(macro_execute(&cancel_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_CANCELLED);
        assert(action_calls == 0 && open_calls == 1 && delay_calls == 0);

        memset(&cancel_macro, 0, sizeof(cancel_macro));
        strcpy(cancel_macro.name, "Cancelamento durante atraso");
        cancel_macro.configured = 1;
        cancel_macro.step_count = 1;
        cancel_macro.steps[0].type = DECK_MACRO_STEP_DELAY;
        cancel_macro.steps[0].delay_ms = 100;
        action_calls = open_calls = delay_calls = 0;
        cancel_now = 0;
        cancel_on_open = 0;
        cancel_on_delay = 1;
        assert(macro_execute(&cancel_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_CANCELLED);
        assert(action_calls == 0 && open_calls == 0 && delay_calls == 1);

        cancel_on_action = cancel_on_open = cancel_on_delay = 0;
        cancel_now = 0;
    }

    /* A Unicode URL must survive validation and reach the open executor unchanged. */
    {
        const char *unicode_url = "https://例え.テスト/路径?q=ação";
        DeckMacro url_macro;
        memset(&url_macro, 0, sizeof(url_macro));
        strcpy(url_macro.name, "Abrir site internacional");
        url_macro.configured = 1;
        url_macro.step_count = 1;
        url_macro.steps[0].type = DECK_MACRO_STEP_OPEN;
        strcpy(url_macro.steps[0].value, unicode_url);

        assert(macro_validate(&url_macro));
        open_calls = 0;
        last_open_target[0] = '\0';
        cancel_now = 0;
        assert(macro_execute(&url_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_OK);
        assert(open_calls == 1);
        assert(strcmp(last_open_target, unicode_url) == 0);
    }

    /* Long international URLs must survive parsing and execution unchanged. */
    {
        const char *prefix = "https://例え.テスト/";
        char long_url[DECK_MACRO_VALUE_MAX];
        char expression[DECK_MACRO_VALUE_MAX + 16];
        size_t prefix_length = strlen(prefix);
        DeckMacro long_url_macro;
        assert(prefix_length + 1200 + 1 < sizeof(long_url));
        memcpy(long_url, prefix, prefix_length);
        memset(long_url + prefix_length, 'u', 1200);
        long_url[prefix_length + 1200] = '\0';
        assert(strlen(long_url) > 1024);
        assert(snprintf(expression, sizeof(expression), "open:%s", long_url) > 0);
        assert(macro_step_parse(expression, &step));
        memset(&long_url_macro, 0, sizeof(long_url_macro));
        strcpy(long_url_macro.name, "URL internacional longa");
        long_url_macro.configured = 1;
        long_url_macro.step_count = 1;
        long_url_macro.steps[0] = step;
        assert(macro_validate(&long_url_macro));
        open_calls = 0;
        assert(macro_execute(&long_url_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_OK);
        assert(open_calls == 1 && strcmp(last_open_target, long_url) == 0);
    }

    /* OPEN targets accept exactly DECK_MACRO_VALUE_MAX - 1 UTF-8 bytes,
       including multibyte characters, and reject the first byte beyond the limit. */
    {
        const char *prefix = "https://example.com/";
        char max_target[DECK_MACRO_VALUE_MAX];
        char expression[DECK_MACRO_VALUE_MAX + 16];
        char oversized_target[DECK_MACRO_VALUE_MAX + 1];
        char oversized_expression[DECK_MACRO_VALUE_MAX + 16];
        size_t prefix_length = strlen(prefix);
        size_t remaining = sizeof(max_target) - 1 - prefix_length;
        size_t offset = 0;
        DeckMacro boundary_macro;

        memcpy(max_target, prefix, prefix_length);
        while (offset + 1 < remaining) {
            max_target[prefix_length + offset] = (char)0xC3;
            max_target[prefix_length + offset + 1] = (char)0xA9;
            offset += 2;
        }
        if (offset < remaining) max_target[prefix_length + offset++] = 'x';
        max_target[prefix_length + offset] = '\0';
        assert(strlen(max_target) == DECK_MACRO_VALUE_MAX - 1);
        assert(snprintf(expression, sizeof(expression), "open:%s", max_target) > 0);
        assert(macro_step_parse(expression, &step));

        memset(&boundary_macro, 0, sizeof(boundary_macro));
        strcpy(boundary_macro.name, "Limite UTF-8");
        boundary_macro.configured = 1;
        boundary_macro.step_count = 1;
        boundary_macro.steps[0] = step;
        assert(macro_validate(&boundary_macro));
        open_calls = 0;
        last_open_target[0] = '\0';
        cancel_now = 0;
        assert(macro_execute(&boundary_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_OK);
        assert(open_calls == 1 && strcmp(last_open_target, max_target) == 0);

        /* The maximum-size value must also survive the v1 file round trip. */
        assert(macro_store_load(&store, "macros-test-apps.cfg"));
        store.macros[0] = boundary_macro;
        assert(macro_store_save(&store));
        assert(macro_store_load(&loaded, "macros-test-apps.cfg"));
        assert(macro_store_count(&loaded) == 1);
        assert(strcmp(macro_store_get(&loaded, 1)->steps[0].value, max_target) == 0);
        remove("macros.cfg");
        remove("macros.cfg.bak");
        remove("macros.cfg.tmp");

        memcpy(oversized_target, prefix, prefix_length);
        memset(oversized_target + prefix_length, 'x',
               DECK_MACRO_VALUE_MAX - prefix_length);
        oversized_target[DECK_MACRO_VALUE_MAX] = '\0';
        assert(strlen(oversized_target) == DECK_MACRO_VALUE_MAX);
        assert(snprintf(oversized_expression, sizeof(oversized_expression),
                        "open:%s", oversized_target) > 0);
        assert(!macro_step_parse(oversized_expression, &step));
    }

    fail_action = 1;
    action_calls = open_calls = delay_calls = 0;
    assert(macro_execute(&macro, test_action, test_open, test_delay, test_cancel, NULL) == DECK_MACRO_EXECUTION_FAILED);
    assert(action_calls == 1 && open_calls == 0 && delay_calls == 0);

    fail_action = 0;
    cancel_now = 1;
    assert(macro_execute(&macro, test_action, test_open, test_delay, test_cancel, NULL) == DECK_MACRO_CANCELLED);
    cancel_now = 0;

    macro.steps[0].type = DECK_MACRO_STEP_ACTION;
    strcpy(macro.steps[0].value, "arbitrary_command");
    assert(!macro_validate(&macro));
    build_valid_macro(&macro);
    macro.steps[2].delay_ms = DECK_MACRO_TOTAL_DELAY_MAX_MS + 1u;
    assert(!macro_validate(&macro));
    build_valid_macro(&macro);
    macro.steps[0].type = DECK_MACRO_STEP_DELAY;
    macro.steps[0].delay_ms = DECK_MACRO_STEP_DELAY_MAX_MS;
    macro.steps[1].type = DECK_MACRO_STEP_DELAY;
    macro.steps[1].delay_ms = DECK_MACRO_STEP_DELAY_MAX_MS;
    macro.steps[2].type = DECK_MACRO_STEP_DELAY;
    macro.steps[2].delay_ms = 1;
    assert(!macro_validate(&macro));

    assert(macro_store_load(&store, "macros-test-apps.cfg"));
    assert(store.loaded && macro_store_count(&store) == 0);
    build_valid_macro(&store.macros[0]);
    assert(macro_store_save(&store));
    assert(file_contains("macros.cfg", "version=1"));
    assert(file_contains("macros.cfg", "step2=open:C:\\Program Files\\OBS Studio\\bin\\应用.exe"));

    assert(macro_store_load(&loaded, "macros-test-apps.cfg"));
    assert(macro_store_count(&loaded) == 1);
    assert(macro_store_get(&loaded, 1) != NULL);
    assert(strcmp(macro_store_get(&loaded, 1)->name, macro.name) == 0);

    strcpy(loaded.macros[0].name, "Macro atualizada");
    assert(macro_store_save(&loaded));
    assert(file_contains("macros.cfg.bak", "action:volume_down"));

    /* A long Unicode executable target is serialized, reloaded and executed intact. */
    {
        const char *prefix = "C:\\Aplicações\\Δ\\";
        char long_target[DECK_MACRO_VALUE_MAX];
        char expression[DECK_MACRO_VALUE_MAX + 16];
        size_t prefix_length = strlen(prefix);
        DeckMacro long_path_macro;
        assert(prefix_length + 1500 + sizeof(".exe") < sizeof(long_target));
        memcpy(long_target, prefix, prefix_length);
        memset(long_target + prefix_length, 'p', 1500);
        memcpy(long_target + prefix_length + 1500, ".exe", sizeof(".exe"));
        assert(strlen(long_target) > 1024);
        assert(snprintf(expression, sizeof(expression), "open:%s", long_target) > 0);
        assert(macro_step_parse(expression, &step));

        memset(&long_path_macro, 0, sizeof(long_path_macro));
        strcpy(long_path_macro.name, "Destino Unicode longo");
        long_path_macro.configured = 1;
        long_path_macro.step_count = 1;
        long_path_macro.steps[0] = step;
        assert(macro_validate(&long_path_macro));
        open_calls = 0;
        assert(macro_execute(&long_path_macro, test_action, test_open, test_delay,
                             test_cancel, NULL) == DECK_MACRO_OK);
        assert(open_calls == 1 && strcmp(last_open_target, long_target) == 0);

        loaded.macros[0].step_count = 1;
        loaded.macros[0].steps[0] = step;
        memset(&loaded.macros[0].steps[1], 0, sizeof(loaded.macros[0].steps) -
               sizeof(loaded.macros[0].steps[0]));
        assert(macro_validate(&loaded.macros[0]));
        assert(macro_store_save(&loaded));
        assert(macro_store_load(&store, "macros-test-apps.cfg"));
        assert(strcmp(store.macros[0].steps[0].value, long_target) == 0);
    }

    {
        static DeckMacroStore future;
        write_text("macros.cfg", "[meta]\nversion=99\n");
        assert(!macro_store_load(&future, "macros-test-apps.cfg"));
        assert(future.read_only);
        assert(!macro_store_save(&future));
        assert(file_contains("macros.cfg", "version=99"));
    }

    /* Duplicate schema metadata and macro fields are ambiguous and must fail closed. */
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[meta]\nversion=1\nmarker=duplicate-meta\n",
        "marker=duplicate-meta");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\nversion=1\nmarker=duplicate-version\n",
        "marker=duplicate-version");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[macro1]\nname=Macro\nstep_count=1\n"
        "step1=action:volume_up\n[macro1]\nmarker=duplicate-section\n",
        "marker=duplicate-section");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[macro1]\nname=Primeira\nname=Segunda\n"
        "step_count=1\nstep1=action:volume_up\nmarker=duplicate-name\n",
        "marker=duplicate-name");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[macro1]\nname=Macro\nstep_count=1\n"
        "step_count=1\nstep1=action:volume_up\nmarker=duplicate-count\n",
        "marker=duplicate-count");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[macro1]\nname=Macro\nstep_count=1\n"
        "step1=action:volume_up\nstep1=action:volume_down\nmarker=duplicate-step\n",
        "marker=duplicate-step");

    /* Unknown syntax is rejected fail-closed and never rewritten. */
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[profile]\nmarker=unknown-section\n",
        "marker=unknown-section");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\nunknown_option=true\nmarker=unknown-meta-key\n",
        "marker=unknown-meta-key");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\n[macro1]\nname=Macro\nstep_count=1\n"
        "step1=action:volume_up\nunknown_option=true\nmarker=unknown-macro-key\n",
        "marker=unknown-macro-key");
    assert_invalid_store_is_read_only(
        "[meta]\nversion=1\nthis-is-not-a-key\nmarker=bare-line\n",
        "marker=bare-line");
    assert_invalid_store_is_read_only(
        "orphan=value\n[meta]\nversion=1\nmarker=outside-section\n",
        "marker=outside-section");
    assert_invalid_store_is_read_only(
        "[macro1]\nname=Macro\nstep_count=1\nstep1=action:volume_up\n"
        "[meta]\nversion=1\nmarker=meta-after-macro\n",
        "marker=meta-after-macro");

    /* Exercise macro storage under a real non-ASCII directory path. */
    {
        const char *unicode_directory = "macro-配置-测试-Δ";
        const char *unicode_config_path = "macro-配置-测试-Δ/apps.cfg";
        wchar_t wide_directory[DECK_PATH_WIDE_MAX];
        wchar_t wide_macro_path[DECK_PATH_WIDE_MAX];
        wchar_t wide_backup_path[DECK_PATH_WIDE_MAX];
        wchar_t wide_temporary_path[DECK_PATH_WIDE_MAX];
        static DeckMacroStore unicode_store;
        static DeckMacroStore unicode_loaded;

        assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                   unicode_directory, -1,
                                   wide_directory, DECK_PATH_WIDE_MAX) > 0);
        assert(CreateDirectoryW(wide_directory, NULL) ||
               GetLastError() == ERROR_ALREADY_EXISTS);

        assert(macro_store_load(&unicode_store, unicode_config_path));
        assert(strcmp(unicode_store.path,
                      "macro-配置-测试-Δ/macros.cfg") == 0);
        build_valid_macro(&unicode_store.macros[0]);
        assert(macro_store_save(&unicode_store));

        strcpy(unicode_store.macros[0].name, "Macro Unicode atualizada");
        assert(macro_store_save(&unicode_store));

        wcscpy(wide_macro_path, wide_directory);
        wcscat(wide_macro_path, L"\\macros.cfg");
        wcscpy(wide_backup_path, wide_macro_path);
        wcscat(wide_backup_path, L".bak");
        wcscpy(wide_temporary_path, wide_macro_path);
        wcscat(wide_temporary_path, L".tmp");
        assert(GetFileAttributesW(wide_macro_path) != INVALID_FILE_ATTRIBUTES);
        assert(GetFileAttributesW(wide_backup_path) != INVALID_FILE_ATTRIBUTES);

        assert(macro_store_load(&unicode_loaded, unicode_config_path));
        assert(macro_store_count(&unicode_loaded) == 1);
        assert(strcmp(macro_store_get(&unicode_loaded, 1)->name,
                      "Macro Unicode atualizada") == 0);

        DeleteFileW(wide_temporary_path);
        DeleteFileW(wide_backup_path);
        DeleteFileW(wide_macro_path);
        assert(RemoveDirectoryW(wide_directory));
    }

    remove("macros.cfg");
    remove("macros.cfg.bak");
    remove("macros.cfg.tmp");
    puts("macro validation, persistence and execution: OK");
    return 0;
}
