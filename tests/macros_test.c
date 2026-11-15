#include "../src/macros.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int action_calls;
static int open_calls;
static int delay_calls;
static int fail_action;
static int cancel_now;
static char last_open_target[DECK_MACRO_VALUE_MAX];

static int test_action(const char *action_id, void *context) {
    (void)context;
    assert(action_id && action_id[0]);
    ++action_calls;
    return fail_action;
}

static int test_open(const char *target, void *context) {
    (void)context;
    assert(target && target[0]);
    assert(strlen(target) < sizeof(last_open_target));
    strcpy(last_open_target, target);
    ++open_calls;
    return 0;
}

static int test_delay(unsigned int milliseconds, void *context) {
    (void)context;
    assert(milliseconds > 0 && milliseconds <= DECK_MACRO_STEP_DELAY_MAX_MS);
    ++delay_calls;
    return 0;
}

static int test_cancel(void *context) {
    (void)context;
    return cancel_now;
}

static int file_contains(const char *path, const char *needle) {
    FILE *file = fopen(path, "rb");
    char buffer[4096];
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

int main(void) {
    DeckMacroStep step;
    DeckMacro macro;
    DeckMacroStore store;
    DeckMacroStore loaded;

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

    build_valid_macro(&macro);
    assert(macro_validate(&macro));

    action_calls = open_calls = delay_calls = 0;
    fail_action = 0;
    cancel_now = 0;
    assert(macro_execute(&macro, test_action, test_open, test_delay, test_cancel, NULL) == DECK_MACRO_OK);
    assert(action_calls == 1 && open_calls == 1 && delay_calls == 1);
    assert(strcmp(last_open_target, "C:\\Program Files\\OBS Studio\\bin\\应用.exe") == 0);

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

    {
        DeckMacroStore future;
        write_text("macros.cfg", "[meta]\nversion=99\n");
        assert(!macro_store_load(&future, "macros-test-apps.cfg"));
        assert(future.read_only);
        assert(!macro_store_save(&future));
        assert(file_contains("macros.cfg", "version=99"));
    }

    remove("macros.cfg");
    remove("macros.cfg.bak");
    remove("macros.cfg.tmp");
    puts("macro validation, persistence and execution: OK");
    return 0;
}
