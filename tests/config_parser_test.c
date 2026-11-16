#include "../src/config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_config(const char *contents) {
    FILE *file = fopen("test-apps.cfg", "w");
    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
}

static int file_contains(const char *path, const char *needle) {
    char line[DECK_CONFIG_LINE_MAX];
    FILE *file = fopen(path, "r");
    int found = 0;
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, needle)) { found = 1; break; }
    }
    fclose(file);
    return found;
}

int main(void) {
    static DeckConfig config;
    static DeckConfig loaded;

    config_set_defaults(&config);
    assert(config.apps[0].configured == 1);
    assert(strcmp(config.apps[0].path, "action:volume_up") == 0);
    assert(config.apps[5].configured == 1);
    assert(strcmp(config.apps[5].path, "action:volume_down") == 0);
    assert(config.apps[6].configured == 0);

    assert(config_load(NULL, &config) == 0);
    assert(config_load("arquivo-que-nao-existe.cfg", &config) == 0);
    assert(config_load("test-apps.cfg", NULL) == 0);

    /* v1: three legacy app slots are preserved at keys 7-9. */
    write_config(
        "# comentario inicial\n"
        "; outro comentario\n"
        "[APP1]\n"
        " name = Editor de texto  \n"
        " path = notepad.exe\n"
        " icon = C:\\Icons\\editor.ico\n"
        "\n"
        "[app2]\n"
        "name=Navegador\n"
        "path=browser.exe\n"
        "[app3]\n"
        "name=Paint\n"
        "path=mspaint.exe\n"
    );
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(config.apps[0].configured == 1);
    assert(strcmp(config.apps[0].path, "action:volume_up") == 0);
    assert(strcmp(config.apps[1].path, "action:audio_mute") == 0);
    assert(config.apps[6].configured == 1);
    assert(strcmp(config.apps[6].name, "Editor de texto") == 0);
    assert(strcmp(config.apps[6].path, "notepad.exe") == 0);
    assert(strcmp(config.apps[6].icon_path, "C:\\Icons\\editor.ico") == 0);
    assert(config.apps[7].configured == 1);
    assert(strcmp(config.apps[7].name, "Navegador") == 0);
    assert(config.apps[8].configured == 1);
    assert(strcmp(config.apps[8].path, "mspaint.exe") == 0);
    assert(strcmp(config.apps[9].path, "action:mic_mute") == 0);
    assert(strcmp(config.apps[14].path, "ms-settings:") == 0);

    /* v1 files with nine sections preserve intentionally empty app4-app9 slots. */
    write_config(
        "[app1]\nname=Primeiro\npath=first.exe\n"
        "[app2]\nname=Segundo\npath=second.exe\n"
        "[app3]\nname=Terceiro\npath=third.exe\n"
        "[app4]\n[app5]\n[app6]\n[app7]\n[app8]\n[app9]\n"
    );
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(strcmp(config.apps[6].name, "Primeiro") == 0);
    assert(config.apps[9].configured == 0);
    assert(config.apps[14].configured == 0);

    /* v2: all nine persisted app slots are shifted intact into keys 7-15. */
    write_config(
        "[meta]\nversion=2\n"
        "[app1]\nname=One\npath=one.exe\n"
        "[app2]\nname=Two\npath=two.exe\n"
        "[app3]\nname=Three\npath=three.exe\n"
        "[app4]\nname=Four\npath=four.exe\n"
        "[app5]\nname=Five\npath=five.exe\n"
        "[app6]\nname=Six\npath=six.exe\n"
        "[app7]\nname=Seven\npath=seven.exe\n"
        "[app8]\nname=Eight\npath=eight.exe\n"
        "[app9]\nname=Nine\npath=nine.exe\n"
    );
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(strcmp(config.apps[0].path, "action:volume_up") == 0);
    assert(strcmp(config.apps[6].name, "One") == 0);
    assert(strcmp(config.apps[14].name, "Nine") == 0);
    assert(strcmp(config.apps[14].path, "nine.exe") == 0);

    write_config("[app1]\nname=Somente nome\n[app2]\npath=sem-nome.exe\n");
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(config.apps[6].configured == 0);
    assert(config.apps[7].configured == 0);

    write_config("");
    assert(config_load("test-apps.cfg", &config) == 1);
    for (int i = 0; i < DECK_KEY_COUNT; ++i) {
        assert(config.apps[i].configured == 0);
        assert(config.apps[i].name[0] == '\0');
        assert(config.apps[i].path[0] == '\0');
    }

    write_config("[meta]\nversion=3\n[meta]\nversion=3\n[app1]\nname=One\npath=one.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "version=3"));

    write_config("[meta]\nversion=3\nversion=3\n[app1]\nname=One\npath=one.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    write_config("[meta]\nversion=3\n[app1]\nname=First\npath=first.exe\n"
                 "[app1]\nname=Second\npath=second.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "name=First"));

    write_config("[meta]\nversion=3\n[app1]\nname=First\nname=Second\npath=first.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    write_config("[meta]\nversion=3\n[app1]\nname=First\npath=first.exe\npath=second.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    write_config("[meta]\nversion=3\n[app1]\nname=First\npath=first.exe\n"
                 "icon=first.ico\nicon=second.ico\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    /* Unknown and misplaced syntax must fail closed instead of being ignored. */
    write_config("[meta]\nversion=3\n[future]\nmarker=unknown-section\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "marker=unknown-section"));

    write_config("[meta]\nversion=3\nfuture_meta=1\n[app1]\nname=One\npath=one.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "future_meta=1"));

    write_config("[meta]\nversion=3\n[app1]\nname=One\npath=one.exe\nfuture_field=1\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "future_field=1"));

    write_config("[meta]\nversion=3\n[app1]\nname=One\npath=one.exe\nline-without-equals\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "line-without-equals"));

    write_config("orphan=value\n[meta]\nversion=3\n[app1]\nname=One\npath=one.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);
    assert(file_contains("test-apps.cfg", "orphan=value"));

    write_config("[app1]\nname=One\npath=one.exe\n[meta]\nversion=3\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    write_config("[app1\nname=One\npath=one.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    write_config("[app10]\nname=Ten\npath=ten.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    /* Reject overlong physical lines instead of parsing a truncated prefix. */
    {
        char long_line[DECK_CONFIG_LINE_MAX + 100];
        memset(long_line, 'x', sizeof(long_line));
        long_line[sizeof(long_line) - 1] = '\0';
        write_config(long_line);
        assert(config_load("test-apps.cfg", &config) == 0);
    }

    /* Reject values that would previously be silently truncated to DECK_TEXT_MAX. */
    {
        char oversized[512];
        const char *prefix = "[meta]\nversion=3\n[app1]\nname=";
        size_t offset = strlen(prefix);
        memcpy(oversized, prefix, offset);
        memset(oversized + offset, 'A', DECK_TEXT_MAX);
        strcpy(oversized + offset + DECK_TEXT_MAX, "\npath=oversized.exe\n");
        write_config(oversized);
        assert(config_load("test-apps.cfg", &config) == 0);
    }

    /* v3 remains readable; the next explicit save upgrades it to v4 with a backup. */
    {
        char long_path[DECK_PATH_TEXT_MAX];
        char long_icon[DECK_PATH_TEXT_MAX];
        char contents[DECK_CONFIG_LINE_MAX * 2];
        int written;
        strcpy(long_path, "C:\\Apps\\");
        for (int i = 0; i < 400; ++i) strcat(long_path, "音");
        strcat(long_path, ".exe");
        strcpy(long_icon, "C:\\Icons\\");
        for (int i = 0; i < 350; ++i) strcat(long_icon, "アイ");
        strcat(long_icon, ".ico");
        written = snprintf(contents, sizeof(contents),
                           "[meta]\nversion=3\n[app1]\nname=Long path\npath=%s\nicon=%s\n",
                           long_path, long_icon);
        assert(written > 0 && (size_t)written < sizeof(contents));
        write_config(contents);
        assert(config_load("test-apps.cfg", &loaded) == 1);
        assert(strlen(loaded.apps[0].path) > DECK_TEXT_MAX);
        assert(strlen(loaded.apps[0].icon_path) > DECK_TEXT_MAX);
        assert(strcmp(loaded.apps[0].path, long_path) == 0);
        assert(strcmp(loaded.apps[0].icon_path, long_icon) == 0);
        assert(config_save("test-apps.cfg", &loaded) == 1);
        assert(file_contains("test-apps.cfg", "version=4"));
        assert(file_contains("test-apps.cfg.bak", "version=3"));
        assert(config_load("test-apps.cfg", &config) == 1);
        assert(strcmp(config.apps[0].path, long_path) == 0);
        assert(strcmp(config.apps[0].icon_path, long_icon) == 0);
    }

    /* Reject path and icon fields above their new UTF-8 byte capacity. */
    {
        char too_long[DECK_CONFIG_LINE_MAX + 64];
        const char *path_prefix = "[meta]\nversion=4\n[app1]\nname=Too long\npath=";
        size_t offset = strlen(path_prefix);
        memcpy(too_long, path_prefix, offset);
        memset(too_long + offset, 'P', DECK_PATH_TEXT_MAX);
        strcpy(too_long + offset + DECK_PATH_TEXT_MAX, "\n");
        write_config(too_long);
        assert(config_load("test-apps.cfg", &config) == 0);

        {
            const char *icon_prefix = "[meta]\nversion=4\n[app1]\nname=Too long\npath=valid.exe\nicon=";
            offset = strlen(icon_prefix);
            memcpy(too_long, icon_prefix, offset);
            memset(too_long + offset, 'I', DECK_PATH_TEXT_MAX);
            strcpy(too_long + offset + DECK_PATH_TEXT_MAX, "\n");
            write_config(too_long);
            assert(config_load("test-apps.cfg", &config) == 0);
        }
    }

    /* Invalid UTF-8 must not enter the in-memory configuration. */
    write_config("[meta]\nversion=3\n[app1]\nname=Invalid \xC3\x28\npath=bad.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    memset(&config, 0, sizeof(config));
    strcpy(config.apps[0].name, "Volume +");
    strcpy(config.apps[0].path, "action:volume_up");
    strcpy(config.apps[6].name, "Editor");
    strcpy(config.apps[6].path, "notepad.exe");
    strcpy(config.apps[6].icon_path, "C:\\Icons\\editor.ico");
    strcpy(config.apps[7].name, "Calculadora");
    strcpy(config.apps[7].path, "calc.exe");
    strcpy(config.apps[14].name, "Atalho quinze");
    strcpy(config.apps[14].path, "example.exe");
    assert(config_save("test-apps-saved.cfg", &config) == 1);
    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(loaded.apps[0].configured == 1);
    assert(strcmp(loaded.apps[0].path, "action:volume_up") == 0);
    assert(loaded.apps[6].configured == 1);
    assert(strcmp(loaded.apps[6].name, "Editor") == 0);
    assert(strcmp(loaded.apps[6].path, "notepad.exe") == 0);
    assert(strcmp(loaded.apps[6].icon_path, "C:\\Icons\\editor.ico") == 0);
    assert(loaded.apps[7].configured == 1);
    assert(loaded.apps[8].configured == 0);
    assert(loaded.apps[14].configured == 1);
    assert(strcmp(loaded.apps[14].path, "example.exe") == 0);
    assert(file_contains("test-apps-saved.cfg", "[meta]"));
    assert(file_contains("test-apps-saved.cfg", "version=4"));
    assert(file_contains("test-apps-saved.cfg", "[app15]"));

    strcpy(config.apps[6].name, "Editor atualizado");
    assert(config_save("test-apps-saved.cfg", &config) == 1);
    assert(file_contains("test-apps-saved.cfg.bak", "name=Editor"));
    assert(file_contains("test-apps-saved.cfg", "name=Editor atualizado"));
    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[6].name, "Editor atualizado") == 0);

    assert(config_save("teste-configuração-测试.cfg", &config) == 1);
    assert(config_load("teste-configuração-测试.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[6].name, "Editor atualizado") == 0);

    strcpy(config.apps[14].name, "Aplicativo 日本語");
    strcpy(config.apps[14].path, "C:\\Apps\\应用.exe");
    assert(config_save("teste-configuração-测试.cfg", &config) == 1);
    assert(config_load("teste-configuração-测试.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[14].name, "Aplicativo 日本語") == 0);
    assert(strcmp(loaded.apps[14].path, "C:\\Apps\\应用.exe") == 0);

    strcpy(config.apps[8].name, "Atalho incompleto");
    assert(config_save("test-apps-saved.cfg", &config) == 0);
    strcpy(config.apps[8].name, "Linha\nInvalida");
    strcpy(config.apps[8].path, "mspaint.exe");
    assert(config_save("test-apps-saved.cfg", &config) == 0);

    /* Save rejects malformed UTF-8 and unterminated fixed-size fields. */
    config.apps[8].name[0] = (char)0xC3;
    config.apps[8].name[1] = '(';
    config.apps[8].name[2] = '\0';
    assert(config_save("test-apps-saved.cfg", &config) == 0);
    assert(file_contains("test-apps-saved.cfg", "name=Editor atualizado"));

    memset(config.apps[8].name, 'X', sizeof(config.apps[8].name));
    assert(config_save("test-apps-saved.cfg", &config) == 0);
    assert(file_contains("test-apps-saved.cfg", "name=Editor atualizado"));

    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(loaded.apps[6].configured == 1);
    assert(strcmp(loaded.apps[6].name, "Editor atualizado") == 0);
    assert(loaded.apps[8].configured == 0);

    remove("test-apps.cfg");
    remove("test-apps-saved.cfg");
    remove("test-apps-saved.cfg.bak");
    remove("teste-configuração-测试.cfg");
    remove("teste-configuração-测试.cfg.bak");
    puts("config parser/save: OK");
    return 0;
}
