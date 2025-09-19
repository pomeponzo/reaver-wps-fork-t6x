#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <time.h>
#endif

static const char *platform_find_basename(const char *path) {
        const char *last = path;
        while (*path) {
                if (*path == '/' || *path == '\\') {
                        last = path + 1;
                }
                ++path;
        }
        return last;
}

const char *platform_basename(const char *path)
{
        if (!path) {
                return "";
        }
        return platform_find_basename(path);
}

#ifdef _WIN32
static int winsock_initialized;

void platform_init(void)
{
        if (!winsock_initialized) {
                WSADATA data;
                int result = WSAStartup(MAKEWORD(2, 2), &data);
                if (result != 0) {
                        fprintf(stderr, "[-] Failed to initialize Winsock (error %d)\n", result);
                        exit(EXIT_FAILURE);
                }
                winsock_initialized = 1;
                atexit(platform_cleanup);
        }
}

void platform_cleanup(void)
{
        if (winsock_initialized) {
                        WSACleanup();
                        winsock_initialized = 0;
        }
}

void platform_sleep_seconds(unsigned int seconds)
{
        Sleep(seconds * 1000U);
}

void platform_sleep_millis(unsigned long millis)
{
        if (millis == 0) {
                millis = 1;
        }
        Sleep((DWORD) millis);
}

void platform_usleep(unsigned long usec)
{
        unsigned long millis = usec / 1000UL;
        if (millis * 1000UL < usec) {
                ++millis;
        }
        platform_sleep_millis(millis);
}

#else

void platform_init(void)
{
        /* Nothing to do on POSIX systems. */
}

void platform_cleanup(void)
{
        /* Nothing to do on POSIX systems. */
}

void platform_sleep_seconds(unsigned int seconds)
{
        sleep(seconds);
}

void platform_sleep_millis(unsigned long millis)
{
        if (millis >= 1000UL) {
                struct timespec req = {
                        .tv_sec = (time_t) (millis / 1000UL),
                        .tv_nsec = (long) ((millis % 1000UL) * 1000000L)
                };
                nanosleep(&req, NULL);
        } else {
                usleep(millis * 1000UL);
        }
}

void platform_usleep(unsigned long usec)
{
        usleep(usec);
}

#endif
