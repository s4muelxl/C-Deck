#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static char *trim(char *s) { char *end; while (*s && isspace((unsigned char)*s)) ++s; if (!*s) return s; end=s+strlen(s)-1; while (end>s && isspace((unsigned char)*end)) *end--='\0'; return s; }
static void copy_value(char *dst,size_t cap,const char *src) { if (!cap) return; strncpy(dst,src,cap-1); dst[cap-1]='\0'; }
int config_load(const char *path,DeckConfig *out_config) {
 FILE *fp; char line[1024]; int current=-1; if (!path || !out_config) return 0; memset(out_config,0,sizeof(*out_config)); fp=fopen(path,"r"); if (!fp) return 0;
 while (fgets(line,sizeof(line),fp)) { char *p=trim(line),*eq; if (*p=='\0'||*p=='#'||*p==';') continue; if (*p=='[') { int n=0; current=(sscanf(p,"[app%d]",&n)==1&&n>=1&&n<=DECK_APP_COUNT)?n-1:-1; continue; } if (current<0||!(eq=strchr(p,'='))) continue; *eq++='\0'; p=trim(p); eq=trim(eq); if (_stricmp(p,"name")==0) copy_value(out_config->apps[current].name,DECK_TEXT_MAX,eq); else if (_stricmp(p,"path")==0) copy_value(out_config->apps[current].path,DECK_TEXT_MAX,eq); }
 fclose(fp); for (int i=0;i<DECK_APP_COUNT;++i) { DeckApp *app=&out_config->apps[i]; app->configured=app->name[0]!='\0'&&app->path[0]!='\0'; } return 1;
}
