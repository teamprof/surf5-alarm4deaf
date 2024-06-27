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
#include <string.h>
#include <vector>
#include "libs/audio/audio_service.h"
#include "libs/base/timer.h"
#include "libs/base/filesystem.h"
#include "libs/tensorflow/audio_models.h"
#include "libs/tensorflow/utils.h"

#include "../ml/AlarmInterpreter.h"
#include "../ml/AudioBridge.h"
#include "../AppContext.h"
#include "../NpuConst.h"
#include "./ThreadInference.h"
#include "./QueueMainM7.h"

// using namespace coralmicro;

static constexpr char testAudioPath[] = "/coralmicro/models/yamnet_test_audio.bin";
// static constexpr char testAudioPath[] = "/models/yamnet_test_audio.bin";

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef SINGLE_INSTANCE_THREAD_INFERENCE
ThreadInference *ThreadInference::_instance = nullptr;

ThreadInference *ThreadInference::getInstance(void)
{
    if (!_instance)
    {
        static ThreadInference instance;
        _instance = &instance;
    }
    return _instance;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_NAME "ThreadInference"
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadInference::ThreadInference() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                                     handlerMap(),
                                     _npuStatus(StatusUnknown),
                                     _taskInitHandle(nullptr),
                                     _timerInference("TimerInference",
                                                     pdMS_TO_TICKS(coralmicro::tensorflow::kYamnetDurationMs),
                                                     [](TimerHandle_t xTimer)
                                                     {
                                                         if (_instance)
                                                         {
                                                             _instance->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                                         }
                                                     })
{
#ifdef SINGLE_INSTANCE_THREAD_INFERENCE
    _instance = this;
#endif

    handlerMap = {
        __EVENT_MAP(ThreadInference, EventSystem),
        __EVENT_MAP(ThreadInference, EventNull), // {EventNull, &ThreadInference::handlerEventNull},
    };
}

///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(ThreadInference, EventSystem, msg) // void ThreadInference::handlerEventSystem(const Message &msg)
{
    // DBGLOG(Debug, "EventSystem(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);

    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        if ((TimerHandle_t)(msg.lParam) == _timerInference.timer())
        {
            schedulerInference();
        }
        else
        {
            DBGLOG(Debug, "unsupported timer handle=0x%08lx", msg.lParam);
        }
        break;

    default:
        DBGLOG(Debug, "Unsupported SystemTriggerSource=%hd", src);
        break;
    }
}
__EVENT_FUNC_DEFINITION(ThreadInference, EventNull, msg) // void ThreadInference::handlerEventNull(const Message &msg)
{
    DBGLOG(Debug, "EventNull(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

///////////////////////////////////////////////////////////////////////
void ThreadInference::onMessage(const Message &msg)
{
    // DBGLOG(Debug, "event=%hd, iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
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

void ThreadInference::start(void *ctx)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    configASSERT(ctx);
    _context = ctx;

    _taskHandle = xTaskCreateStatic(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer);
}

void ThreadInference::setup(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    // ThreadBase::setup() invokes delayInit()
    // do not call ThreadBase::setup() if delayInit() requires large stack size,
    ThreadBase::setup();

    // vTaskDelay(pdMS_TO_TICKS(500));
}

void ThreadInference::run(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());
    ThreadBase::run();
}

// note: this function is invoked in ThreadBase::setup()
void ThreadInference::delayInit(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    if (!AlarmInterpreter::init())
    {
        DBGLOG(Error, "AlarmInterpreter::init() failed!");
        return;
    }
    DBGLOG(Debug, "AlarmInterpreter::init() success");

    ///////////////////////////////////////////////////////////////////////////
    // Run tensorflow on test input file.
    static std::vector<uint8_t> yamnet_test_input_bin;
    if (!coralmicro::LfsReadFile(testAudioPath, &yamnet_test_input_bin))
    {
        DBGLOG(Error, "Failed to load test input!");
        return;
    }
    if (yamnet_test_input_bin.size() !=
        coralmicro::tensorflow::kYamnetAudioSize * sizeof(int16_t))
    {
        DBGLOG(Error, "Input audio size doesn't match expected");
        return;
    }
    auto interpreter = AlarmInterpreter::getInstance();
    if (interpreter)
    {
        auto input_tensor = interpreter->getInterpreter()->input_tensor(0);
        std::memcpy(tflite::GetTensorData<uint8_t>(input_tensor),
                    yamnet_test_input_bin.data(), yamnet_test_input_bin.size());
        interpreter->inference();
    }
    ///////////////////////////////////////////////////////////////////////////

    // Setup audio
    AudioBridge::init();
    // vTaskDelay(pdMS_TO_TICKS(coralmicro::tensorflow::kYamnetDurationMs));

    // start schedular for inference
    _timerInference.start();

    _npuStatus = StatusRunning;
    AppContext *appContext = static_cast<AppContext *>(context());
    // DBGLOG(Debug, "appContext=%p, appContext->queueMain=%p", appContext, appContext->queueMain);
    if (appContext)
    {
        auto thread = reinterpret_cast<QueueMainM7 *>(appContext->queueMain);
        DBGLOG(Debug, "appContext->queueMain=%p, thread=%p", appContext->queueMain, thread);
        postEvent(thread, EventIPC, EventNpuStatus, _npuStatus);
    }
}

///////////////////////////////////////////////////////////////////////
void ThreadInference::schedulerInference(void)
{
    // DBGLOG(Debug, "schedulerInference()");
    auto interpreter = AlarmInterpreter::getInstance();
    if (interpreter)
    {
        AudioBridge::accessLatestSamples();
        interpreter->inference();
    }
}
