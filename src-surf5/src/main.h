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
#include <stdio.h>
#include "w7500x.h"

///////////////////////////////////////////////////////////////////////////////
#define EV_TIMER_1HZ 0x00000001
#define EV_TIMER_2HZ 0x00000002
#define EV_GPIO_NPU 0x00000004
#define EV_ALARM_ON 0x00000010
#define EV_ALARM_OFF 0x00000020

///////////////////////////////////////////////////////////////////////////////
#define SOCKET_NUM_DHCP 0
#define SOCKET_NUM_DNS 1
#define SOCKET_NUM_WEB 2

///////////////////////////////////////////////////////////////////////////////
// extern void delay(__IO uint32_t milliseconds);

///////////////////////////////////////////////////////////////////////////////
#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

///////////////////////////////////////////////////////////////////////////////

/* Uncomment the line below according to the target EVB board of W7500x used in
 your application
 */
#if !defined(USE_WIZWIKI_W7500_EVAL) && !defined(USE_WIZWIKI_W7500P_EVAL) && !defined(USE_WIZWIKI_W7500_ECO_EVAL)
// #define USE_WIZWIKI_W7500_EVAL
// #define USE_WIZWIKI_W7500P_EVAL
// #define USE_WIZWIKI_W7500_ECO_EVAL
// #define USE_MY_EVAL
#if defined(USE_WIZWIKI_W7500_EVAL)
#define USING_UART1
#else
#define USING_UART2
#endif
#endif
