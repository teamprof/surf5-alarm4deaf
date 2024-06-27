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
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/timers.h"

class SoftwareTimer
{
public:
    SoftwareTimer(const char *pcTimerName, TickType_t xTimerPeriodInTicks,
                  BaseType_t xAutoReload, void *const pvTimerID,
                  TimerCallbackFunction_t pxCallbackFunction)
    {
        _hTimer = xTimerCreate(pcTimerName,
                               xTimerPeriodInTicks,
                               xAutoReload,
                               pvTimerID,
                               pxCallbackFunction);
    }

    ~SoftwareTimer()
    {
        xTimerDelete(_hTimer, 0);
        _hTimer = nullptr;
    }

    static SoftwareTimer *create(TimerCallbackFunction_t pxCallbackFunction, uint32_t interval_in_ms)
    {
        return new SoftwareTimer("TimerPeriodic",
                                 pdMS_TO_TICKS(interval_in_ms),
                                 pdTRUE, // auto-reload when expire.
                                 nullptr,
                                 pxCallbackFunction);
    }

    void start(void)
    {
        if (xTimerIsTimerActive(_hTimer) == pdFALSE)
        {
            xTimerStart(_hTimer, 0);
        }
    }

    void stop(void)
    {
        if (xTimerIsTimerActive(_hTimer) != pdFALSE)
        {
            xTimerStop(_hTimer, 0);
        }
    }

    TimerHandle_t timer(void)
    {
        return _hTimer;
    }

private:
    TimerHandle_t _hTimer;
};
