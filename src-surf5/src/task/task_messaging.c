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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "w7500x.h"
#include "socket.h"
#include "task_messaging.h"
#include "../main.h"
#include "../net_util.h"
#include "../AppLog.h"
#include "../../secret.h"

#define CONNECT_TIMEOUT 30 // in unit of seconds
#define AVAILABLE_PORT_START 50000

// for release
static const char _apiHost[] = CALLMEBOT_HOST;
static const char _apiPath[] = CALLMEBOT_PATH;
static const int _apiPort = CALLMEBOT_PORT;

// // for test only
// static const char _apiHost[] = "www.google.com";
// static const char _apiPath[] = "/search?q=";
// static const int _apiPort = CALLMEBOT_PORT;

static uint8_t _hostIp[4] = {0};

#define SHARE_BUF_SIZE 2048
static char _share_buf[SHARE_BUF_SIZE];

static ClientState _clientState = ClientReady;
static uint16_t _connectTimeout = 0;
static bool _isHttpStatusLineReceived = false;

static MessagingCallback _messagingCallback = NULL;

#define MESSAGING_CALLBACK(x)     \
    if (_messagingCallback)       \
    {                             \
        (*_messagingCallback)(x); \
    }

static bool encode_url(const char *url, char *dest, int size)
{
    static const char hex[] = "0123456789ABCDEF"; // Hexadecimal characters for encoding

    int destIndex = 0;
    for (size_t srcIndex = 0; srcIndex < strlen(url); ++srcIndex)
    {
        char c = url[srcIndex];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            // Safe characters, leave them unchanged
            dest[destIndex++] = c;
        }
        else
        {
            // Encode special characters
            dest[destIndex++] = '%';
            dest[destIndex++] = hex[(c >> 4) & 0xF];
            dest[destIndex++] = hex[c & 0xF];
        }
        // if (isalnum(url[srcIndex]) || url[srcIndex] == '-' || url[srcIndex] == '_' || url[srcIndex] == '.' || url[srcIndex] == '~')
        // {
        //     // Safe characters, leave them unchanged
        //     dest[destIndex++] = url[srcIndex];
        // }
        // else
        // {
        //     // Encode special characters
        //     dest[destIndex++] = '%';
        //     dest[destIndex++] = hex[(url[srcIndex] >> 4) & 0xF];
        //     dest[destIndex++] = hex[url[srcIndex] & 0xF];
        // }
        if (destIndex >= size)
        {
            return false;
        }
    }
    dest[destIndex] = '\0'; // Null-terminate the encoded URL
    return true;
}

static bool prepare_http_content(const char *text)
{
    // DBGLOG(Debug, "text=%s", text);

    strcpy(_share_buf, "GET ");
    strncat(_share_buf, _apiPath, sizeof(_share_buf) - sizeof("GET "));
    int length = strlen(_share_buf);
    // strncat(_share_buf, text, sizeof(_share_buf) - length - 1);
    if (!encode_url(text, &_share_buf[length], sizeof(_share_buf) - length - 1))
    {
        DBGLOG(Debug, "encode_url() failed");
        return false;
    }
    length = strlen(_share_buf);
    strncat(_share_buf, " HTTP/1.1\r\nHost: ", sizeof(_share_buf) - length - 1);
    strncat(_share_buf, _apiHost, sizeof(_share_buf) - length - sizeof(" HTTP/1.1\r\nHost: "));
    length = strlen(_share_buf);
    strncat(_share_buf, "\r\nConnection: close\r\n\r\n", sizeof(_share_buf) - length - 1);
    // DBGLOG(Debug, "content length=%d, _share_buf=\r\n%s", strlen(_share_buf), _share_buf);
    return true;
}

static bool write_http_content(const char *httpContent)
{
    // DBGLOG(Debug, "httpContent=\r\n%s", httpContent);
    int32_t ret = send(SOCKET_NUM_WEB, (uint8_t *)httpContent, strlen(httpContent) + 1);
    if (ret <= SOCK_ERROR)
    {
        DBGLOG(Debug, "send() return error: %ld", ret);
        return false;
    }
    return true;
}

static bool read_http_response(int *ptr_response_code)
{
    uint16_t size = 0;
    while ((size = getSn_RX_RSR(SOCKET_NUM_WEB)) > 0)
    {
        size = min(size, sizeof(_share_buf));
        int32_t ret = recv(SOCKET_NUM_WEB, (uint8_t *)_share_buf, size);
        if (ret <= SOCK_ERROR)
        {
            break;
        }

        DBGLOG(Debug, "recv() returns %ld, size=%d", ret, size);
        // DBGLOG(Debug, "_share_buf=\r\n%s",  _share_buf);
        // DBGLOG(Debug, "recv() returns %ld, size=%d, _share_buf=\r\n%s", ret, size, _share_buf);
        if (!_isHttpStatusLineReceived)
        {
            char *http_code_str = strstr((char *)_share_buf, "HTTP/");
            if (http_code_str)
            {
                _isHttpStatusLineReceived = true;

                int http_code;
                sscanf(http_code_str, "HTTP/%*d.%*d %d", &http_code);
                DBGLOG(Debug, "HTTP Status Code: %d", http_code);

                *ptr_response_code = http_code;
                return true;
            }
        }
    }

    DBGLOG(Debug, "getSn_SR(SOCKET_NUM_WEB)=0x%02x", getSn_SR(SOCKET_NUM_WEB));

    // bool isSocketClosed = getSn_SR(SOCKET_NUM_WEB) == SOCK_CLOSED;
    // DBGLOG(Debug, "isSocketClosed=%d", isSocketClosed);

    return false;
}

static uint16_t get_available_port(void)
{
    static uint16_t available_port = AVAILABLE_PORT_START;
    uint16_t port = available_port;
    available_port = ((available_port + 1) % 1024) + AVAILABLE_PORT_START;
    return port;
}

bool send_whatsapp(const char *text)
{
    switch (_clientState)
    {
    case ClientReady:
    {
        if (is_ip_null(_hostIp))
        {
            DBGLOG(Debug, "_hostIp is null");
            return false;
        }

        if (!prepare_http_content(text))
        {
            DBGLOG(Debug, "prepare_http_content() return false");
            return false;
        }
        // DBGLOG(Debug, "_share_buf=\r\n%s", _share_buf);

        uint16_t any_port = get_available_port();
        if (socket(SOCKET_NUM_WEB, Sn_MR_TCP, any_port, 0x00) != SOCKET_NUM_WEB)
        {
            DBGLOG(Debug, "socket(%d, Sn_MR_TCP, %u, 0x00) failed", SOCKET_NUM_WEB, any_port);
            return false;
        }

        DBGLOG(Debug, "connecting to the %d.%d.%d.%d:%d ...", _hostIp[0], _hostIp[1], _hostIp[2], _hostIp[3], _apiPort);
        if (connect(SOCKET_NUM_WEB, _hostIp, _apiPort) != SOCK_OK)
        {
            DBGLOG(Debug, "connect(%d, %d.%d.%d.%d, %d) failed", SOCKET_NUM_WEB, _hostIp[0], _hostIp[1], _hostIp[2], _hostIp[3], _apiPort);
            close(SOCKET_NUM_WEB);
            return false;
        }

        _clientState = ClientConnecting;
        _connectTimeout = 0;
        _isHttpStatusLineReceived = false;
        MESSAGING_CALLBACK(MessageConnecting);

        return true;
        break;
    }

    default:
        DBGLOG(Debug, "unsupported _clientState=%d", (uint16_t)(_clientState));
        break;
    }

    return false;
}

char *get_share_buf(void)
{
    return _share_buf;
}

const char *get_host_name(void)
{
    return _apiHost;
}

void set_host_ip(const uint8_t *ip)
{
    memcpy(_hostIp, ip, sizeof(_hostIp));
    DBGLOG(Debug, "hostIp=%d.%d.%d.%d", _hostIp[0], _hostIp[1], _hostIp[2], _hostIp[3]);
}

void task_messaging(void)
{
    ClientState state = _clientState;
    switch (state)
    {
    case ClientConnecting:
        if ((getSn_SR(SOCKET_NUM_WEB) == SOCK_ESTABLISHED) && (getSn_IR(SOCKET_NUM_WEB) & Sn_IR_CON))
        {
            DBGLOG(Debug, "connected to %d.%d.%d.%d:%d", _hostIp[0], _hostIp[1], _hostIp[2], _hostIp[3], _apiPort);

            setSn_IR(SOCKET_NUM_WEB, Sn_IR_CON);
            write_http_content(_share_buf);
            _clientState = ClientConnected;
            _connectTimeout = 0;
            MESSAGING_CALLBACK(MessageSending);
        }
        else if (_connectTimeout++ >= CONNECT_TIMEOUT)
        {
            DBGLOG(Debug, "connecting to the %d.%d.%d.%d:%d timeout!", _hostIp[0], _hostIp[1], _hostIp[2], _hostIp[3], _apiPort);

            disconnect(SOCKET_NUM_WEB);
            close(SOCKET_NUM_WEB);
            _clientState = ClientReady;
            MESSAGING_CALLBACK(MessageSentFail);
        }
        break;

    case ClientConnected:
    {
        int response_code;
        if (read_http_response(&response_code))
        {
            // if (response_code == 200)
            // {
            //     DBGLOG(Debug, "Sent success");
            //     if (_messagingCallback)
            //     {
            //         (*_messagingCallback)(MessageSentSuccess);
            //     }
            // }
            // else
            // {
            //     DBGLOG(Error, "read_http_response(): response_code=%d", response_code);
            //     if (_messagingCallback)
            //     {
            //         (*_messagingCallback)(MessageSentFail);
            //     }
            // }

            // DBGLOG(Debug, "getSn_SR(SOCKET_NUM_WEB)=0x%02x", getSn_SR(SOCKET_NUM_WEB));

            close(SOCKET_NUM_WEB);
            _clientState = ClientReady;

            DBGLOG(Debug, "%s", (response_code == 200) ? "MessageSentSuccess" : "MessageSentFail");
            MessageStatus status = (response_code == 200) ? MessageSentSuccess : MessageSentFail;
            MESSAGING_CALLBACK(status);

            // DBGLOG(Debug, "getSn_SR(SOCKET_NUM_WEB)=0x%02x", getSn_SR(SOCKET_NUM_WEB));
        }
        else if (getSn_SR(SOCKET_NUM_WEB) == SOCK_CLOSED)
        {
            DBGLOG(Debug, "getSn_SR(SOCKET_NUM_WEB) == SOCK_CLOSED");
            disconnect(SOCKET_NUM_WEB);
            close(SOCKET_NUM_WEB);
            _clientState = ClientReady;
            MESSAGING_CALLBACK(MessageSentFail);
        }
        else if (_connectTimeout++ >= CONNECT_TIMEOUT)
        {
            disconnect(SOCKET_NUM_WEB);
            close(SOCKET_NUM_WEB);
            _clientState = ClientReady;
            MESSAGING_CALLBACK(MessageSentFail);
        }
        break;
    }

    case ClientReady:
        // nothing to do
        break;

    default:
        DBGLOG(Debug, "unsupported _clientState=%d", (uint16_t)(state));
        break;
    }
}

void set_messaging_callback(MessagingCallback cb)
{
    _messagingCallback = cb;
}