/* Parser smoke test. Compile with src/config.c on Windows. */
#include "../src/config.h"
#include <assert.h>
#include <stdio.h>
int main(void){DeckConfig c;FILE *f=fopen("test-apps.cfg","w");assert(f);fputs("[app1]\nname=Editor\npath=notepad.exe\n[app2]\nname=Browser\npath=browser.exe\n",f);fclose(f);assert(config_load("test-apps.cfg",&c)==1);assert(c.apps[0].configured);assert(c.apps[1].configured);assert(!c.apps[2].configured);remove("test-apps.cfg");puts("config parser: OK");return 0;}
