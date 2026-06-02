#include "core/thread_pool.h"
#include <stdlib.h>

#ifdef _WIN32
static DWORD WINAPI worker_fn(LPVOID arg)
#else
static void *worker_fn(void *arg)
#endif
{
    ThreadPool *p=(ThreadPool *)arg;
    for(;;){
        mutex_lock(&p->mutex);
        while(p->queue_size==0&&!p->shutdown) cond_wait(&p->work_ready,&p->mutex);
        if(p->shutdown&&p->queue_size==0){mutex_unlock(&p->mutex);break;}
        Task *t=p->head; p->head=t->next; if(!p->head)p->tail=NULL;
        p->queue_size--; p->active++;
        mutex_unlock(&p->mutex);
        t->fn(t->arg); free(t);
        mutex_lock(&p->mutex); p->active--;
        if(p->active==0&&p->queue_size==0) cond_broadcast(&p->work_done);
        mutex_unlock(&p->mutex);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

ThreadPool *pool_create(int n) {
    ThreadPool *p=(ThreadPool *)calloc(1,sizeof(ThreadPool));
    p->threads=(thread_t *)malloc((size_t)n*sizeof(thread_t)); p->count=n;
    mutex_init(&p->mutex); cond_init(&p->work_ready); cond_init(&p->work_done);
    for(int i=0;i<n;i++){
#ifdef _WIN32
        p->threads[i]=CreateThread(NULL,0,worker_fn,p,0,NULL);
#else
        pthread_create(&p->threads[i],NULL,worker_fn,p);
#endif
    }
    return p;
}

void pool_submit(ThreadPool *p, task_fn fn, void *arg) {
    Task *t=(Task *)malloc(sizeof(Task)); t->fn=fn; t->arg=arg; t->next=NULL;
    mutex_lock(&p->mutex);
    if(p->tail) p->tail->next=t; else p->head=t;
    p->tail=t; p->queue_size++; cond_signal(&p->work_ready);
    mutex_unlock(&p->mutex);
}

void pool_destroy(ThreadPool *p) {
    mutex_lock(&p->mutex); p->shutdown=1; cond_broadcast(&p->work_ready); mutex_unlock(&p->mutex);
    for(int i=0;i<p->count;i++){
#ifdef _WIN32
        WaitForSingleObject(p->threads[i],INFINITE); CloseHandle(p->threads[i]);
#else
        pthread_join(p->threads[i],NULL);
#endif
    }
    cond_destroy(&p->work_ready); cond_destroy(&p->work_done);
    mutex_destroy(&p->mutex); free(p->threads); free(p);
}
