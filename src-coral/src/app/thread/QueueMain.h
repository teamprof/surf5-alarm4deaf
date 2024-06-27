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
#include "../../lib/arduprof/ArduProf.h"

#include "../AppEvent.h"

// #define SINGLE_INSTANCE_QUEUE_MAIN

class QueueMain : public MessageBus
{
public:
#ifdef SINGLE_INSTANCE_QUEUE_MAIN
    static QueueMain *getInstance(void)
    {
        return _instance;
    }
#else
    QueueMain();
#endif // SINGLE_INSTANCE_QUEUE_MAIN

private:
#ifdef SINGLE_INSTANCE_QUEUE_MAIN
    QueueMain();
    static QueueMain *_instance;
#endif // SINGLE_INSTANCE_QUEUE_MAIN

public:
    virtual void start(void *);
    // void run(void);

protected:
    typedef void (QueueMain::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

    virtual void onMessage(const Message &msg);

private:
    TaskHandle_t taskInitHandle;

    virtual void setup(void);
    virtual void delayInit(void);

    ///////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};
