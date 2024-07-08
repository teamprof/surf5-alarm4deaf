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

typedef enum _ClientState
{
    ClientUnknown = 0,
    ClientReady,
    ClientConnecting,
    ClientConnected,
} ClientState;

typedef enum _MessageStatus
{
    MessageUnknown = 0,
    MessageConnecting,
    MessageSending,
    MessageSentSuccess,
    MessageSentFail,
} MessageStatus;

extern char *get_share_buf(void);
extern const char *get_host_name(void);
extern void set_host_ip(const uint8_t *ip);
extern void task_messaging(void);
extern bool send_whatsapp(const char *s);

typedef void (*MessagingCallback)(const MessageStatus status);
extern void set_messaging_callback(MessagingCallback cb);