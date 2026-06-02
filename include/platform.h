#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <io.h>
    typedef SOCKET sock_t;
    #define INVALID_SOCK INVALID_SOCKET
    #define sock_close   closesocket
    typedef HANDLE             thread_t;
    typedef CRITICAL_SECTION   mutex_t;
    typedef CONDITION_VARIABLE cond_t;
    #define mutex_init(m)    InitializeCriticalSection(m)
    #define mutex_lock(m)    EnterCriticalSection(m)
    #define mutex_unlock(m)  LeaveCriticalSection(m)
    #define mutex_destroy(m) DeleteCriticalSection(m)
    #define cond_init(c)      InitializeConditionVariable(c)
    #define cond_wait(c,m)    SleepConditionVariableCS((c),(m),INFINITE)
    #define cond_signal(c)    WakeConditionVariable(c)
    #define cond_broadcast(c) WakeAllConditionVariable(c)
    #define cond_destroy(c)   ((void)0)
    #define strcasecmp  _stricmp
    #define strncasecmp _strnicmp
    #define strdup      _strdup
    #define strcasestr  _strcasestr_win
    static __inline char *_strcasestr_win(const char *h, const char *n) {
        if (!*n) return (char *)h;
        for (; *h; h++) if (_strnicmp(h, n, strlen(n)) == 0) return (char *)h;
        return NULL;
    }
    #define sleep(s) Sleep((s)*1000)
    #define usleep(us) Sleep((us)/1000 ? (us)/1000 : 1)
#else
    #define _GNU_SOURCE
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <errno.h>
    typedef int             sock_t;
    #define INVALID_SOCK    (-1)
    #define sock_close      close
    typedef pthread_t       thread_t;
    typedef pthread_mutex_t mutex_t;
    typedef pthread_cond_t  cond_t;
    #define mutex_init(m)    pthread_mutex_init((m), NULL)
    #define mutex_lock(m)    pthread_mutex_lock(m)
    #define mutex_unlock(m)  pthread_mutex_unlock(m)
    #define mutex_destroy(m) pthread_mutex_destroy(m)
    #define cond_init(c)      pthread_cond_init((c), NULL)
    #define cond_wait(c,m)    pthread_cond_wait((c),(m))
    #define cond_signal(c)    pthread_cond_signal(c)
    #define cond_broadcast(c) pthread_cond_broadcast(c)
    #define cond_destroy(c)   pthread_cond_destroy(c)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_PATH_LEN 1024

#endif /* PLATFORM_H */
