#ifndef PORTABLE_UTILS_H
#define PORTABLE_UTILS_H

#include "stdio.h"   // printf
#include "stdbool.h" // true
#include "stdlib.h"  // exit
#include "limits.h"
#include "math.h"

static inline size_t size_t_max(size_t a, size_t b) { return a > b ? a : b; }
static inline size_t size_t_min(size_t a, size_t b) { return a < b ? a : b; }
static inline size_t size_t_clamp(size_t min, size_t max, size_t value) {
    return size_t_max(min, size_t_min(max, value)); }
static inline int int_max(int a, int b) { return a > b ? a : b; }
static inline int int_min(int a, int b) { return a < b ? a : b; }
static inline int int_clamp(int min, int max, int value) {
    return int_max(min, int_min(max, value)); }
static inline float float_clamp(float min, float max, float value) {
    return fmaxf(min, fminf(max, value)); }

// Source - https://stackoverflow.com/a/1068937
int int_digit_places (int n) {
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
       Add more ifs as needed and adjust this final return as well. */
    return 10;
}


#ifdef WIN32
#include <windows.h>
#else
#include <time.h> // nanosleep
#endif
void sleep_ms(int milliseconds){
#ifdef WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

#define ASSERT(value) \
    do { \
        if ((value) != true) { \
            printf("\nFAILED ASSERT %s|%s:%d\n", \
                    __func__, __FILE__, __LINE__); \
            printf(#value); \
            printf("\n"); \
            asm("int3"); \
            exit(1); \
        } \
    } while (0)

#define printfd(fmt, ...) \
    do { \
        printf(fmt, ##__VA_ARGS__); \
        printf(" %s:%s:%d\n", __func__, __FILE__, __LINE__); \
    } while (0)

#define PRIbool(arg) (arg ? "true" : "false")

#endif
