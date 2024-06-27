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
#include <map>

#include "../../base/ipc/IpcCoreM7.h"
#include "../peripheral/i2c/I2cSlave.h"
#include "../AppEvent.h"
#include "../NpuConst.h"
#include "./QueueMain.h"

#define SINGLE_INSTANCE_QUEUE_MAIN_M7

#define I2C_TX_BUF_SIZE 16

class QueueMainM7 final : public QueueMain
{
public:
#ifdef SINGLE_INSTANCE_QUEUE_MAIN_M7
    static QueueMainM7 *getInstance(void);
#else
    QueueMainM7();
#endif // SINGLE_INSTANCE_QUEUE_MAIN_M7

private:
#ifdef SINGLE_INSTANCE_QUEUE_MAIN_M7
    static QueueMainM7 *_instance;
    QueueMainM7();
#endif // SINGLE_INSTANCE_QUEUE_MAIN_M7

public:
    void start(void *context, void *ipc);

protected:
    typedef void (QueueMainM7::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

    void onMessage(const Message &msg);

private:
    IpcCoreM7 *ipc;
    uint8_t _npuStatus;
    uint8_t _npuResult;
    I2cSlave _i2cDevice;
    uint8_t _i2cReg;
    uint8_t _i2cTxBufStatus[I2C_TX_BUF_SIZE];
    uint8_t _i2cTxBufResult[I2C_TX_BUF_SIZE];
    uint8_t _i2cTxBufNull[I2C_TX_BUF_SIZE];
    PeriodicTimer _timer1Hz;

    virtual void setup(void);

    void setNpuStatus(uint8_t status);
    void setNpuResult(uint8_t result);

    void printMainInfo(void);

    ///////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventI2C)
    __EVENT_FUNC_DECLARATION(EventIPC)
    __EVENT_FUNC_DECLARATION(EventICC)
    __EVENT_FUNC_DECLARATION(EventSystem)
    // __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};
