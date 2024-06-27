/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <stdint.h>

// enable debug log by defining the following macro
#define DEBUG_LOG_LEVEL Debug
// disable debug log by comment out the macro DEBUG_LOG_LEVEL
// #undef DEBUG_LOG_LEVEL

enum LogLevel
{
    Error = 0,
    Info,
    Debug
};

extern void debugLog(const char *file, int line, const char *func,
                     uint16_t logLevel, const char *msg, ...);

#define PRINT(msg, ...)             \
    {                               \
        printf(msg, ##__VA_ARGS__); \
    }

#define PRINTLN(msg, ...)           \
    {                               \
        printf(msg, ##__VA_ARGS__); \
        printf("\r\n");             \
    }

#ifdef DEBUG_LOG_LEVEL
#define DBGLOG(logLevel, msg, ...)                                 \
    if (logLevel <= DEBUG_LOG_LEVEL)                               \
    {                                                              \
        printf("[%s line %d] %s: ", __FILE__, __LINE__, __func__); \
        printf(msg, ##__VA_ARGS__);                                \
        printf("\r\n");                                            \
    }
/*
#define DBGLOG_ARRAY(logLevel, title, array, length)                         \
    if (logLevel <= DEBUG_LOG_LEVEL)                                         \
    {                                                                        \
        printf("[%s line %hd] %s: %s", __FILE__, __LINE__, __func__, title); \
        printf("\r\n");                                                      \
    }
*/
/*
#define DBGLOG_ARRAY(logLevel, title, array, length)                         \
    if (logLevel <= DEBUG_LOG_LEVEL)                                         \
    {                                                                        \
        printf("[%s line %hd] %s: %s", __FILE__, __LINE__, __func__, title); \
        for (size_t i = 0; i < length; i++)                                  \
        {                                                                    \
            printf("%02x ", (array[i]));                                     \
        }                                                                    \
        printf("\r\n");                                                      \
    }
*/
#else
#define DBGLOG(x)
#endif
