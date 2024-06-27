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
#include <optional>
#include "libs/base/gpio.h"
#include "libs/base/utils.h"
#include "libs/camera/camera.h"
#include "libs/base/led.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"

#include "../AppContext.h"
#include "../AppEvent.h"
#include "../NpuConst.h"
#include "./QueueMainM7.h"

// using namespace coralmicro;

////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef SINGLE_INSTANCE_QUEUE_MAIN_M7
QueueMainM7 *QueueMainM7::_instance = nullptr;

QueueMainM7 *QueueMainM7::getInstance(void)
{
    if (!_instance)
    {
        static QueueMainM7 instance;
        _instance = &instance;
    }
    return _instance;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////
QueueMainM7::QueueMainM7() : QueueMain(),
                             handlerMap(),
                             ipc(nullptr),
                             _npuStatus(StatusUnknown),
                             _npuResult(ResultUnknown),
                             _i2cDevice(coralmicro::I2c::kI2c1),
                             _timer1Hz("Timer1Hz",
                                       pdMS_TO_TICKS(1000),
                                       [](TimerHandle_t xTimer)
                                       {
                                           if (_instance)
                                           {
                                               _instance->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                           }
                                       })

{
    memset(_i2cTxBufStatus, 0, sizeof(_i2cTxBufStatus));
    setNpuStatus(StatusUnknown);
    memset(_i2cTxBufResult, 0, sizeof(_i2cTxBufResult));
    memset(_i2cTxBufNull, 0, sizeof(_i2cTxBufNull));

    handlerMap = {
        __EVENT_MAP(QueueMainM7, EventI2C),
        __EVENT_MAP(QueueMainM7, EventIPC),
        __EVENT_MAP(QueueMainM7, EventICC),
        __EVENT_MAP(QueueMainM7, EventSystem),
    };
}

///////////////////////////////////////////////////////////////////////
// event handler
///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(QueueMainM7, EventI2C, msg) // void QueueMainM7::handlerEventI2C(const Message &msg)
{
    // DBGLOG(Debug, "EventI2C(%hd), iParam=0x%04x, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);

    lpi2c_slave_transfer_event_t ev = static_cast<lpi2c_slave_transfer_event_t>(msg.iParam);
    if (!(ev & kLPI2C_SlaveAllEvents))
    {
        DBGLOG(Debug, "unsupported lpi2c_slave_transfer_event=0x%04x", msg.iParam);
        return;
    }

    if (ev & kLPI2C_SlaveAddressMatchEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveAddressMatchEvent");
    }
    if (ev & kLPI2C_SlaveTransmitEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveTransmitEvent: uParam=%d, lParam=0x%08lx", msg.uParam, msg.lParam);
    }
    if (ev & kLPI2C_SlaveReceiveEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveReceiveEvent: uParam=%d, lParam=0x%08lx", msg.uParam, msg.lParam);
    }
    if (ev & kLPI2C_SlaveTransmitAckEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveTransmitAckEvent");
    }
    if (ev & kLPI2C_SlaveRepeatedStartEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveRepeatedStartEvent");
    }
    if (ev & kLPI2C_SlaveCompletionEvent)
    {
        DBGLOG(Debug, "kLPI2C_SlaveCompletionEvent");
    }
}
__EVENT_FUNC_DEFINITION(QueueMainM7, EventIPC, msg) // void QueueMainM7::handlerEventIPC(const Message &msg)
{
    DBGLOG(Debug, "EventIPC(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    IpcEvent ipcEvent = static_cast<IpcEvent>(msg.iParam);
    switch (ipcEvent)
    {
    case EventNpuStatus:
    {
        uint8_t npuStatus = (uint8_t)msg.uParam;
        setNpuStatus(npuStatus);
        break;
    }

    case EventNpuResult:
    {
        uint8_t npuResult = (uint8_t)msg.uParam;
        setNpuResult(npuResult);
        break;
    }

    default:
        DBGLOG(Debug, "Unsupported ipcEvent=%hd, uParam=%hu, lParam=%lu", ipcEvent, msg.uParam, msg.lParam);
        break;
    }
}

__EVENT_FUNC_DEFINITION(QueueMainM7, EventICC, msg) // void QueueMainM7::handlerEventICC(const Message &msg)
{
    DBGLOG(Debug, "EventICC(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

__EVENT_FUNC_DEFINITION(QueueMainM7, EventSystem, msg) // void QueueMainM7::handlerEventSystem(const Message &msg)
{
    // DBGLOG(Debug, "EventSystem(%hd), iParam=%hd, uParam=%hu, lParam=%lu", msg.event, msg.iParam, msg.uParam, msg.lParam);
    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        if ((TimerHandle_t)(msg.lParam) == _timer1Hz.timer())
        {
            // DBGLOG(Debug, "_timer1Hz");
        }
        else
        {
            DBGLOG(Debug, "unsupported timer handle=0x%08lx", msg.lParam);
        }
        break;

    default:
        DBGLOG(Debug, "unsupported SystemTriggerSource=%d", src);
        break;
    }
}

///////////////////////////////////////////////////////////////////////
void QueueMainM7::onMessage(const Message &msg)
{
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        QueueMain::onMessage(msg);
    }
}

///////////////////////////////////////////////////////////////////////
void QueueMainM7::start(void *context, void *ipc)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    this->ipc = static_cast<IpcCoreM7 *>(ipc);
    QueueMain::start(context);

    printMainInfo();
}

void QueueMainM7::setup(void)
{
    DBGLOG(Debug, "xPortGetFreeHeapSize()=%u", xPortGetFreeHeapSize());

    GpioConfigureInterrupt(
        Gpio::kUserButton, GpioInterruptMode::kIntModeFalling,
        [=]()
        {
            this->postEvent(EventSystem);
        },
        50 * 1e3 // debounce_interval_us
    );

    _i2cDevice.init([=](I2cConfig *config, lpi2c_target_transfer_t *transfer) mutable
                    {
                        switch (transfer->event)
                        {
                        case kLPI2C_SlaveTransmitEvent:
                            if (this->_i2cReg == CommandGetStatus)
                            {
                                transfer->data = this->_i2cTxBufStatus;
                                transfer->dataSize = sizeof(this->_i2cTxBufStatus);
                            }
                            else if (this->_i2cReg == CommandGetResult)
                            {
                                transfer->data = this->_i2cTxBufResult;
                                transfer->dataSize = sizeof(this->_i2cTxBufResult);
                            }
                            else
                            {
                                transfer->data = this->_i2cTxBufNull;
                                transfer->dataSize = sizeof(this->_i2cTxBufNull);
                            }
                            break;
                        case kLPI2C_SlaveReceiveEvent:
                            transfer->data = &_i2cReg;
                            transfer->dataSize = sizeof(_i2cReg);
                            break;
                        default:
                            transfer->data = this->_i2cTxBufNull;
                            transfer->dataSize = sizeof(this->_i2cTxBufNull);
                            break;
                        }
                        this->postEvent(EventI2C, transfer->event);
                        //
                    });

    _timer1Hz.start();
    // _timer1Hz.stop();

    // // ThreadBase::setup() invokes delayInit()
    // // do not call ThreadBase::setup() if delayInit() requires large stack size,
    // ThreadBase::setup();
}

///////////////////////////////////////////////////////////////////////
void QueueMainM7::setNpuStatus(uint8_t status)
{
    _npuStatus = status;
    _i2cTxBufStatus[0] = status;
    LedSet(Led::kUser, status == StatusRunning);
    DBGLOG(Debug, "EventNpuStatus: _npuStatus=%hd", _npuStatus);
}

void QueueMainM7::setNpuResult(uint8_t result)
{
    _npuResult = result;
    _i2cTxBufResult[0] = result;
    LedSet(Led::kUser, result == ResultAlarmOn);
    DBGLOG(Debug, "EventNpuResult: _npuResult=%hd", _npuResult);
}

void QueueMainM7::printMainInfo(void)
{
    uint64_t uniqueId = GetUniqueId();
    std::string serialNumber = GetSerialNumber();
    std::string usb_ip;
    if (!GetUsbIpAddress(&usb_ip))
    {
        usb_ip = "null";
    }

    PRINTLN("===============================================================================");
    PRINTLN("GetUniqueId()=0x%08x%08x", (unsigned int)(uniqueId >> 32), (unsigned int)(uniqueId));
    PRINTLN("GetSerialNumber()=%s", serialNumber.c_str());
    PRINTLN("GetUsbIpAddress()=%s", usb_ip.c_str());
    PRINTLN("===============================================================================");
}