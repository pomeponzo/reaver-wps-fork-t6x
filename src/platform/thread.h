#ifndef REAVER_PLATFORM_THREAD_H
#define REAVER_PLATFORM_THREAD_H

#include <stddef.h>

typedef void *(*platform_thread_fn)(void *context);

#ifdef _WIN32
#include <windows.h>

struct platform_thread {
        HANDLE handle;
        void *context;
};

typedef struct platform_thread platform_thread_t;

#else
#include <pthread.h>

typedef struct platform_thread {
        pthread_t handle;
} platform_thread_t;

#endif

int platform_thread_create(platform_thread_t *thread, platform_thread_fn fn, void *arg);
int platform_thread_join(platform_thread_t *thread, void **result);

#endif /* REAVER_PLATFORM_THREAD_H */
