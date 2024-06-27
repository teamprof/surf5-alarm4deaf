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
#include "libs/base/gpio.h"

#include "./QueueMain.h"
#include "../AppContext.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SINGLE_INSTANCE_QUEUE_MAIN
QueueMain *QueueMain::_instance = nullptr;
#endif

QueueMain::QueueMain() : MessageBus(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         handlerMap()
{
#ifdef SINGLE_INSTANCE_QUEUE_MAIN
    _instance = this;
#endif

    handlerMap = {
        __EVENT_MAP(QueueMain, EventNull), // {EventNull, &QueueMain::handlerEventNull},
    };
}

///////////////////////////////////////////////////////////////////////
// event handler
///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(QueueMain, EventNull, msg) // void QueueMain::handlerEventNull(const Message &msg)
{
    DBGLOG(Debug, "EventNull(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

void QueueMain::onMessage(const Message &msg)
{
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        DBGLOG(Debug, "Unsupported event=%hd, iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    }
}

void QueueMain::start(void *ctx)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    MessageBus::start(ctx);
    setup();
}

void QueueMain::setup(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    // // ThreadBase::setup() invokes delayInit()
    // // do not call ThreadBase::setup() if delayInit() requires large stack size,
    // ThreadBase::setup();
}

// this function is invoked by "ThreadBase::setup()"
void QueueMain::delayInit(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    //////////////////////////////////////////////////////////////
    // add time consuming init code here
    //////////////////////////////////////////////////////////////
}
