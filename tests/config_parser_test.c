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

int main(void) {
    DeckConfig config;

    assert(config_load(NULL, &config) == 0);
    assert(config_load("arquivo-que-nao-existe.cfg", &config) == 0);
    assert(config_load("test-apps.cfg", NULL) == 0);

    write_config(
        "# comentario inicial\n"
        "; outro comentario\n"
        "[APP1]\n"
        " name = Editor de texto  \n"
        " path = notepad.exe\n"
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
    assert(config.apps[1].configured == 1);
    assert(strcmp(config.apps[1].name, "Navegador") == 0);
    assert(config.apps[2].configured == 0);

    write_config("[app1]\nname=Somente nome\n[app2]\npath=sem-nome.exe\n");
    assert(config_load("test-apps.cfg", &config) == 1);
    assert(config.apps[0].configured == 0);
    assert(config.apps[1].configured == 0);
    assert(config.apps[2].configured == 0);

    write_config("");
    assert(config_load("test-apps.cfg", &config) == 1);
    for (int i = 0; i < DECK_APP_COUNT; ++i) {
        assert(config.apps[i].configured == 0);
        assert(config.apps[i].name[0] == '\0');
        assert(config.apps[i].path[0] == '\0');
    }

    remove("test-apps.cfg");
    puts("config parser: OK (validacao, espacos, comentarios e secoes)");
    return 0;
}
