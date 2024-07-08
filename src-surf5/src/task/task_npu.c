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
#include "../main.h"
#include "../soft_i2c.h"
#include "../AppLog.h"
#include "pins.h"
#include "task_npu.h"

#define I2C_ADDR 0x55
#define I2C_CLOCK 100000L

static bool isNpuRunning = false;
static uint8_t lastNpuResult = ResultUnknown;

///////////////////////////////////////////////////////////////////////////////
bool task_npu_init(void)
{
    isNpuRunning = false;
    lastNpuResult = ResultUnknown;

    return true;
}

unsigned int task_npu(unsigned int ev)
{
    unsigned int ret = 0;

    if (ev & EV_TIMER_1HZ)
    {
        if (!isNpuRunning)
        {
            uint8_t status = 0;
            int size = i2c_read_register(I2C_ADDR, CommandGetStatus, &status, sizeof(status));
            DBGLOG(Debug, "EV_TIMER_1HZ: i2c_read_register(): size=%d, status=0x%02x", size, status);
            if (size > 0 && status == StatusRunning)
            {
                isNpuRunning = true;
            }
            else
            {
                isNpuRunning = false;
            }
        }
    }

    if (ev & EV_TIMER_2HZ)
    {
        if (isNpuRunning)
        {
            uint8_t result;
            int size = i2c_read_register(I2C_ADDR, CommandGetResult, &result, sizeof(result));
            DBGLOG(Debug, "EV_TIMER_2HZ: i2c_read_register(): size=%d, result=0x%02x", size, result);
            if (size > 0)
            {
                if (lastNpuResult != result)
                {
                    lastNpuResult = result;
                    if (result == ResultAlarmOn)
                    {
                        ret = EV_ALARM_ON;
                    }
                    else
                    {
                        ret = EV_ALARM_OFF;
                    }
                }
                isNpuRunning = true;
            }
            else
            {
                isNpuRunning = false;
            }
        }
    }

    return ret;
}