#include "store/rset.h"
#include <stdlib.h>
#include <string.h>

static unsigned int hm(const char *s) {
    unsigned int h=5381; for(;*s;s++) h=((h<<5)+h)^(unsigned char)*s; return h%RSET_BUCKETS;
}

RSet *rset_new(void) {
    RSet *s=(RSet *)calloc(1,sizeof(RSet));
    s->table=(RSetMember **)calloc(RSET_BUCKETS,sizeof(RSetMember*));
    return s;
}

void rset_free(RSet *s) {
    for(int i=0;i<RSET_BUCKETS;i++){
        RSetMember *m=s->table[i];
        while(m){RSetMember *nx=m->next;free(m->member);free(m);m=nx;}
    }
    free(s->table); free(s);
}

int rset_add(RSet *s, const char *member) {
    unsigned int b=hm(member);
    for(RSetMember *m=s->table[b];m;m=m->next)
        if(strcmp(m->member,member)==0) return 0;
    RSetMember *nm=(RSetMember *)calloc(1,sizeof(RSetMember));
    nm->member=strdup(member); nm->next=s->table[b]; s->table[b]=nm; s->count++; return 1;
}

int rset_rem(RSet *s, const char *member) {
    unsigned int b=hm(member);
    RSetMember **pp=&s->table[b];
    while(*pp){
        if(strcmp((*pp)->member,member)==0){
            RSetMember *d=*pp; *pp=d->next; free(d->member); free(d); s->count--; return 1;
        }
        pp=&(*pp)->next;
    }
    return 0;
}

int rset_ismember(RSet *s, const char *member) {
    unsigned int b=hm(member);
    for(RSetMember *m=s->table[b];m;m=m->next)
        if(strcmp(m->member,member)==0) return 1;
    return 0;
}

char **rset_members(RSet *s, int *n) {
    char **arr=(char **)malloc((size_t)s->count*sizeof(char*));
    int i=0;
    for(int b=0;b<RSET_BUCKETS;b++)
        for(RSetMember *m=s->table[b];m;m=m->next) arr[i++]=m->member;
    *n=i; return arr;
}
