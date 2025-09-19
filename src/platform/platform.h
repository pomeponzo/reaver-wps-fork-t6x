#ifndef REAVER_PLATFORM_H
#define REAVER_PLATFORM_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

void platform_init(void);
void platform_cleanup(void);
const char *platform_basename(const char *path);
void platform_sleep_seconds(unsigned int seconds);
void platform_sleep_millis(unsigned long millis);
void platform_usleep(unsigned long usec);

#endif /* REAVER_PLATFORM_H */
