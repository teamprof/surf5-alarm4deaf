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
#include <cstdio>
#include "libs/base/mutex.h"
#include "libs/base/watchdog.h"
#include "libs/base/led.h"
#include "libs/base/utils.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

#include "./src/lib/arduprof/ArduProf.h"
#include "./src/app/AppContext.h"
#include "./src/app/thread/QueueMainM7.h"
#include "./src/app/thread/ThreadInference.h"
#include "./src/app/rpc/RpcServer.h"

///////////////////////////////////////////////////////////////////////////////
static AppContext appContext = {0};

///////////////////////////////////////////////////////////////////////////////

static void createTasks(void)
{
#ifdef SINGLE_INSTANCE_QUEUE_MAIN_M7
  appContext.queueMain = QueueMainM7::getInstance();
#else
  static QueueMainM7 queueMain;
  appContext.queueMain = &queueMain;
#endif
  configASSERT(appContext.queueMain);

#ifdef SINGLE_INSTANCE_THREAD_INFERENCE
  appContext.threadInference = ThreadInference::getInstance();
#else
  static ThreadInference threadInference;
  appContext.threadInference = &threadInference;
#endif

  if (appContext.queueMain)
  {
    // DBGLOG(Debug, "appContext.queueMain=%p", appContext.queueMain);
    static_cast<QueueMainM7 *>(appContext.queueMain)->start(&appContext, IpcCoreM7::getSingleton());
  }

  if (appContext.threadInference)
  {
    appContext.threadInference->start(&appContext);
  }

  // start RPC server
  auto rpcServer = RpcServer::getInstance();
  rpcServer->start(nullptr, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
extern "C" void app_main(void *param)
// extern "C" [[noreturn]] void app_main(void *param)
{
  (void)param;

  vTaskDelay(pdMS_TO_TICKS(1000)); // let USB becomes ready
  DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

  constexpr WatchdogConfig wdt_config = {
      .timeout_s = 8,
      .pet_rate_s = 3,
      .enable_irq = false,
  };
  WatchdogStart(wdt_config);
  // WatchdogStop();

  createTasks();

  (reinterpret_cast<QueueMain *>(appContext.queueMain))->messageLoopForever();

  // should not be here
  DBGLOG(Error, "should NOT be here");
  vTaskDelay(pdMS_TO_TICKS(100));

  vTaskSuspend(nullptr);
  // vTaskDelete(nullptr);
}
