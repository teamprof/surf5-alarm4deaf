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
#include "third_party/freertos_kernel/include/task.h"
#include "third_party/freertos_kernel/include/queue.h"

#include "../MessageQueue.h"

class ThreadBase : public MessageBus
{
public:
    ThreadBase(uint16_t queueLength,
               uint8_t *pucQueueStorageBuffer = nullptr,
               StaticQueue_t *pxQueueBuffer = nullptr) : MessageBus(queueLength, pucQueueStorageBuffer, pxQueueBuffer),
                                                         _taskHandle(nullptr)
    {
        if (pucQueueStorageBuffer != nullptr && pxQueueBuffer != nullptr)
        {
            _queue = xQueueCreateStatic(queueLength, sizeof(Message), pucQueueStorageBuffer, pxQueueBuffer);
        }
        else
        {
            _queue = xQueueCreate(queueLength, sizeof(Message));
        }
        configASSERT(_queue != NULL);
    }

    virtual void start(void *) = 0;

    // virtual void onMessage(const Message &msg) = 0;

    virtual void run(void)
    {
        setup();

        messageLoopForever();

        vTaskDelay(pdMS_TO_TICKS(100)); // delay 100ms
        vTaskDelete(_taskHandle);
    }

protected:
    virtual void setup(void)
    {
        // BaseType_t result = xTimerPendFunctionCall(
        //     [](void *param1, uint32_t param2)
        //     {
        //         // LOG_TRACE("xTimerPendFunctionCall()");
        //         static_cast<ThreadBase *>(param1)->delayInit();
        //     },
        //     this,        // param1
        //     (uint32_t)0, // param2
        //     pdMS_TO_TICKS(200));
        // (void)result;
        // // LOG_TRACE("xTimerPendFunctionCall() returns ", result);
        delayInit();
    }
    // virtual void setup(void) = 0;

    virtual void delayInit(void) = 0;

    TaskHandle_t _taskHandle;
};