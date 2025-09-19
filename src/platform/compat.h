#ifndef REAVER_PLATFORM_COMPAT_H
#define REAVER_PLATFORM_COMPAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef ssize_t
typedef SSIZE_T ssize_t;
#endif

#define mkdir(path, mode) _mkdir(path)
#define unlink _unlink
#define read _read
#define write _write
#define close _close
#endif

#endif /* REAVER_PLATFORM_COMPAT_H */
