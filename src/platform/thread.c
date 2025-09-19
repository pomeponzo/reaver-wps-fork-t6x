#include "thread.h"

#include <stdlib.h>

#ifdef _WIN32
#include <process.h>

struct thread_context {
        platform_thread_fn fn;
        void *arg;
        void *result;
};

static unsigned __stdcall platform_thread_trampoline(void *param)
{
        struct thread_context *ctx = param;
        ctx->result = ctx->fn(ctx->arg);
        return 0;
}

int platform_thread_create(platform_thread_t *thread, platform_thread_fn fn, void *arg)
{
        if (!thread || !fn) {
                return -1;
        }

        struct thread_context *ctx = calloc(1, sizeof(*ctx));
        if (!ctx) {
                return -1;
        }
        ctx->fn = fn;
        ctx->arg = arg;

        thread->handle = (HANDLE) _beginthreadex(NULL, 0, platform_thread_trampoline, ctx, 0, NULL);
        if (!thread->handle) {
                free(ctx);
                return -1;
        }
        thread->context = ctx;
        return 0;
}

int platform_thread_join(platform_thread_t *thread, void **result)
{
        if (!thread || !thread->handle) {
                return -1;
        }

        WaitForSingleObject(thread->handle, INFINITE);
        CloseHandle(thread->handle);
        thread->handle = NULL;

        struct thread_context *ctx = thread->context;
        if (result) {
                *result = ctx ? ctx->result : NULL;
        }
        free(ctx);
        thread->context = NULL;
        return 0;
}

#else

int platform_thread_create(platform_thread_t *thread, platform_thread_fn fn, void *arg)
{
        if (!thread || !fn) {
                return -1;
        }
        return pthread_create(&thread->handle, NULL, fn, arg);
}

int platform_thread_join(platform_thread_t *thread, void **result)
{
        if (!thread) {
                return -1;
        }
        return pthread_join(thread->handle, result);
}

#endif
