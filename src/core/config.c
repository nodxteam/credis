#include "core/config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void config_defaults(Config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->port       = 6379;
    strncpy(cfg->bind_addr, "0.0.0.0", sizeof(cfg->bind_addr)-1);
    strncpy(cfg->aof_file,  "credis.aof", sizeof(cfg->aof_file)-1);
    cfg->aof_enabled = 1;
    cfg->max_clients = 128;
    cfg->timeout_sec = 0;
    cfg->hz          = 10;
    strncpy(cfg->loglevel, "notice", sizeof(cfg->loglevel)-1);
}

static void trim(char *s) {
    size_t i=0,len=strlen(s);
    while(i<len&&isspace((unsigned char)s[i])) i++;
    if(i) memmove(s,s+i,len-i+1);
    len=strlen(s);
    while(len>0&&isspace((unsigned char)s[len-1])) s[--len]='\0';
}

int config_load(Config *cfg, const char *path) {
    FILE *fp=fopen(path,"r"); if(!fp) return 0;
    char line[512];
    while(fgets(line,sizeof(line),fp)) {
        line[strcspn(line,"\r\n")]='\0';
        if(line[0]=='#'||line[0]=='\0') continue;
        char *eq=strchr(line,' '); if(!eq) continue;
        *eq='\0'; char *key=line, *val=eq+1; trim(key); trim(val);
        if     (strcmp(key,"port")==0)        cfg->port        = atoi(val);
        else if(strcmp(key,"bind")==0)        strncpy(cfg->bind_addr,val,63);
        else if(strcmp(key,"appendonly")==0)  cfg->aof_enabled = (strcmp(val,"yes")==0);
        else if(strcmp(key,"appendfilename")==0) strncpy(cfg->aof_file,val,MAX_PATH_LEN-1);
        else if(strcmp(key,"maxclients")==0)  cfg->max_clients = atoi(val);
        else if(strcmp(key,"timeout")==0)     cfg->timeout_sec = atoi(val);
        else if(strcmp(key,"hz")==0)          cfg->hz          = atoi(val);
        else if(strcmp(key,"loglevel")==0)    strncpy(cfg->loglevel,val,15);
    }
    fclose(fp); return 1;
}

void config_print(const Config *cfg) {
    printf("  port         = %d\n", cfg->port);
    printf("  bind         = %s\n", cfg->bind_addr);
    printf("  appendonly   = %s\n", cfg->aof_enabled?"yes":"no");
    printf("  appendfile   = %s\n", cfg->aof_file);
    printf("  maxclients   = %d\n", cfg->max_clients);
    printf("  hz           = %d\n", cfg->hz);
}
