#ifndef PORTABLE_UTILS_H
#define PORTABLE_UTILS_H

#include "stdio.h"   // printf
#include "stdbool.h" // bool, true
#include "stdlib.h"  // exit
#include "limits.h"
#include "math.h"

#ifdef WIN32
#include <windows.h>
#else
#include <time.h> // nanosleep
#endif
static void sleep_ms(int milliseconds){
#ifdef WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

#ifdef _WIN32
#include <windows.h>
#else // Linux, macOS, etc.
#include <sys/time.h>
#endif

static long get_system_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else // Linux, macOS, etc.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

static inline size_t size_t_max(size_t a, size_t b)                     { return a > b ? a : b; }
static inline size_t size_t_min(size_t a, size_t b)                     { return a < b ? a : b; }
static inline size_t size_t_clamp(size_t min, size_t max, size_t value) { return size_t_max(min, size_t_min(max, value)); }
static inline int int_max(int a, int b)                                 { return a > b ? a : b; }
static inline int int_min(int a, int b)                                 { return a < b ? a : b; }
static inline int int_clamp(int min, int max, int value)                { return int_max(min, int_min(max, value)); }
static inline float float_clamp(float min, float max, float value)      { return fmaxf(min, fminf(max, value)); }
static inline bool int_in_range_inclusive(int min, int max, int value)  { return value >= min && value <= max; }

static int int_digit_places (int n) {
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1
        Add more ifs as needed and adjust this final return as well.
        https://stackoverflow.com/a/1068937
    */
    return 10;
}

#define PYTHON_MODULO(n, M) ((((n) % (M)) + (M)) % (M))

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_GRE "\033[32m"
#define ANSI_YEL "\033[33m"
#define ANSI_BLU "\033[34m"
#define ANSI_MAG "\033[35m"
#define ANSI_CYA "\033[36m"

#define ASSERT(value) \
    do { \
        if (!!(value) != true) { \
            printf("\nFAILED ASSERT %s|%s:%d\n", \
                    __func__, __FILE__, __LINE__); \
            printf(#value); \
            printf("\n"); \
            __asm__("int3"); \
            exit(1); \
        } \
    } while (0)

#define wassert ASSERT

#define static_assert _Static_assert

#define printfd(fmt, ...) \
    do { \
        printf(ANSI_YEL fmt, ##__VA_ARGS__); \
        printf(ANSI_RESET" %s:%s:%d\n", __func__, __FILE__, __LINE__); \
    } while (0)

#define PRIbool(arg) (arg ? "true" : "false")

#endif
