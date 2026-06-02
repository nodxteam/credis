#include "store/rhash.h"
#include <stdlib.h>
#include <string.h>

static unsigned int hf(const char *s) {
    unsigned int h=5381; for(;*s;s++) h=((h<<5)+h)^(unsigned char)*s; return h%RHASH_BUCKETS;
}

RHash *rhash_new(void) {
    RHash *h=(RHash *)calloc(1,sizeof(RHash));
    h->table=(RHashField **)calloc(RHASH_BUCKETS,sizeof(RHashField *));
    return h;
}

void rhash_free(RHash *h) {
    for(int i=0;i<RHASH_BUCKETS;i++){
        RHashField *f=h->table[i];
        while(f){RHashField *nx=f->next;free(f->field);free(f->value);free(f);f=nx;}
    }
    free(h->table); free(h);
}

int rhash_set(RHash *h, const char *field, const char *value) {
    unsigned int b=hf(field);
    for(RHashField *f=h->table[b];f;f=f->next)
        if(strcmp(f->field,field)==0){free(f->value);f->value=strdup(value);return 0;}
    RHashField *nf=(RHashField *)calloc(1,sizeof(RHashField));
    nf->field=strdup(field); nf->value=strdup(value);
    nf->next=h->table[b]; h->table[b]=nf; h->count++; return 1;
}

const char *rhash_get(RHash *h, const char *field) {
    unsigned int b=hf(field);
    for(RHashField *f=h->table[b];f;f=f->next)
        if(strcmp(f->field,field)==0) return f->value;
    return NULL;
}

int rhash_del(RHash *h, const char *field) {
    unsigned int b=hf(field);
    RHashField **pp=&h->table[b];
    while(*pp){
        if(strcmp((*pp)->field,field)==0){
            RHashField *d=*pp; *pp=d->next;
            free(d->field); free(d->value); free(d); h->count--; return 1;
        }
        pp=&(*pp)->next;
    }
    return 0;
}

int rhash_exists(RHash *h, const char *field) { return rhash_get(h,field)!=NULL; }

char **rhash_keys(RHash *h, int *n) {
    char **arr=(char **)malloc((size_t)h->count*sizeof(char*));
    int i=0;
    for(int b=0;b<RHASH_BUCKETS;b++)
        for(RHashField *f=h->table[b];f;f=f->next) arr[i++]=f->field;
    *n=i; return arr;
}

char **rhash_vals(RHash *h, int *n) {
    char **arr=(char **)malloc((size_t)h->count*sizeof(char*));
    int i=0;
    for(int b=0;b<RHASH_BUCKETS;b++)
        for(RHashField *f=h->table[b];f;f=f->next) arr[i++]=f->value;
    *n=i; return arr;
}
