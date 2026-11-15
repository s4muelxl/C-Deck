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
    char line[512];
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
    DeckConfig config;
    DeckConfig loaded;

    assert(config_load(NULL, &config) == 0);
    assert(config_load("arquivo-que-nao-existe.cfg", &config) == 0);
    assert(config_load("test-apps.cfg", NULL) == 0);

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
        "[secao-invalida]\n"
        "name=Ignorado\n"
        "path=ignorado.exe\n"
        "[app3]\n"
        "name=\n"
        "path=mspaint.exe\n"
    );

    assert(config_load("test-apps.cfg", &config) == 1);
    assert(config.apps[0].configured == 1);
    assert(strcmp(config.apps[0].name, "Editor de texto") == 0);
    assert(strcmp(config.apps[0].path, "notepad.exe") == 0);
    assert(strcmp(config.apps[0].icon_path, "C:\\Icons\\editor.ico") == 0);
    assert(config.apps[1].configured == 1);
    assert(strcmp(config.apps[1].name, "Navegador") == 0);
    assert(config.apps[2].configured == 0);
    assert(config.apps[3].configured == 1);
    assert(strcmp(config.apps[3].path, "action:mic_mute") == 0);

    /* Existing nine-slot files from the previous build must keep empty extras empty. */
    write_config(
        "[app1]\nname=Primeiro\npath=first.exe\n"
        "[app2]\nname=Segundo\npath=second.exe\n"
        "[app3]\nname=Terceiro\npath=third.exe\n"
        "[app4]\n[app5]\n[app6]\n[app7]\n[app8]\n[app9]\n"
    );
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(strcmp(config.apps[0].name, "Primeiro") == 0);
    assert(config.apps[3].configured == 0);
    assert(config.apps[8].configured == 0);

    write_config("[app1]\nname=Somente nome\n[app2]\npath=sem-nome.exe\n");
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(config.apps[0].configured == 0);
    assert(config.apps[1].configured == 0);
    assert(config.apps[2].configured == 0);

    write_config("");
    assert(config_load("test-apps.cfg", &config) == 1);
    for (int i = 0; i < 3; ++i) {
        assert(config.apps[i].configured == 0);
        assert(config.apps[i].name[0] == '\0');
        assert(config.apps[i].path[0] == '\0');
    }

    write_config("[meta]\nversion=99\n[app1]\nname=Future\npath=future.exe\n");
    assert(config_load("test-apps.cfg", &config) == 0);

    memset(&config, 0, sizeof(config));
    strcpy(config.apps[0].name, "Editor");
    strcpy(config.apps[0].path, "notepad.exe");
    strcpy(config.apps[0].icon_path, "C:\\Icons\\editor.ico");
    strcpy(config.apps[1].name, "Calculadora");
    strcpy(config.apps[1].path, "calc.exe");
    strcpy(config.apps[8].name, "Atalho nove");
    strcpy(config.apps[8].path, "example.exe");
    assert(config_save("test-apps-saved.cfg", &config) == 1);
    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(loaded.apps[0].configured == 1);
    assert(strcmp(loaded.apps[0].name, "Editor") == 0);
    assert(strcmp(loaded.apps[0].path, "notepad.exe") == 0);
    assert(strcmp(loaded.apps[0].icon_path, "C:\\Icons\\editor.ico") == 0);
    assert(loaded.apps[1].configured == 1);
    assert(strcmp(loaded.apps[1].name, "Calculadora") == 0);
    assert(loaded.apps[2].configured == 0);
    assert(loaded.apps[3].configured == 0);
    assert(loaded.apps[8].configured == 1);
    assert(strcmp(loaded.apps[8].path, "example.exe") == 0);
    assert(file_contains("test-apps-saved.cfg", "[meta]"));
    assert(file_contains("test-apps-saved.cfg", "version=2"));

    /* A subsequent save must leave the previous valid file in .bak. */
    strcpy(config.apps[0].name, "Editor atualizado");
    assert(config_save("test-apps-saved.cfg", &config) == 1);
    assert(file_contains("test-apps-saved.cfg.bak", "name=Editor"));
    assert(file_contains("test-apps-saved.cfg", "name=Editor atualizado"));
    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[0].name, "Editor atualizado") == 0);

    /* UTF-8 paths must survive file creation and loading on Windows. */
    assert(config_save("teste-configuração-测试.cfg", &config) == 1);
    assert(config_load("teste-configuração-测试.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[0].name, "Editor atualizado") == 0);

    strcpy(config.apps[8].name, "Aplicativo 日本語");
    strcpy(config.apps[8].path, "C:\\Apps\\应用.exe");
    assert(config_save("teste-configuração-测试.cfg", &config) == 1);
    assert(config_load("teste-configuração-测试.cfg", &loaded) == 1);
    assert(strcmp(loaded.apps[8].name, "Aplicativo 日本語") == 0);
    assert(strcmp(loaded.apps[8].path, "C:\\\\Apps\\\\应用.exe") == 0);

    strcpy(config.apps[2].name, "Atalho incompleto");
    assert(config_save("test-apps-saved.cfg", &config) == 0);
    strcpy(config.apps[2].name, "Linha\nInvalida");
    strcpy(config.apps[2].path, "mspaint.exe");
    assert(config_save("test-apps-saved.cfg", &config) == 0);

    assert(config_load("test-apps-saved.cfg", &loaded) == 1);
    assert(loaded.apps[0].configured == 1);
    assert(strcmp(loaded.apps[0].name, "Editor atualizado") == 0);
    assert(loaded.apps[2].configured == 0);

    remove("test-apps.cfg");
    remove("test-apps-saved.cfg");
    remove("test-apps-saved.cfg.bak");
    remove("teste-configuração-测试.cfg");
    remove("teste-configuração-测试.cfg.bak");
    puts("config parser/save: OK");
    return 0;
}
