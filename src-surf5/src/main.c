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
#include <stdbool.h>
#include <string.h>
#include "wizchip_conf.h"
#include "dhcp.h"
#include "dns.h"

#include "main.h"
#include "net_util.h"
#include "AppLog.h"
#include "pins.h"
#include "soft_i2c.h"
#include "ssd1306.h"
#include "./task/task_messaging.h"
#include "./task/task_npu.h"
#include "../secret.h"

static const uint8_t MAC_ADDR[] = {0x00, 0x08, 0xDC, 0x01, 0x02, 0x03};

#define DHCP_RETRY_COUNT 3 // number of retries for DHCP run

static wiz_NetInfo gWIZNETINFO;
static bool is_ip_assigned = false;
static bool is_dns_ip_found = false;

#define USER_LED_ON() GPIO_ResetBits(USER_LED_PORT, USER_LED_PIN)
#define USER_LED_OFF() GPIO_SetBits(USER_LED_PORT, USER_LED_PIN)

#ifndef portDISABLE_INTERRUPTS
#define portDISABLE_INTERRUPTS() __asm volatile(" cpsid i " ::: "memory")
#endif

#ifndef portENABLE_INTERRUPTS
#define portENABLE_INTERRUPTS() __asm volatile(" cpsie i " ::: "memory")
#endif

///////////////////////////////////////////////////////////////////////////////
#define LCD_COL 128
#define LCD_ROW 64

#define NUM_CHAR_PER_COL 16
#define NUM_CHAR_PER_ROW 8
static char dpCharBuf[NUM_CHAR_PER_ROW][NUM_CHAR_PER_COL + 1] = {0};
static uint32_t gSysRunTime = 0;

// row 0~1: status
#define DISPLAY_ROW_APP_STATUS0 0   // row 0
#define DISPLAY_ROW_APP_STATUS1 1   // row 1
#define DISPLAY_ROW_SYS_RUN_TIME0 2 // row 2
#define DISPLAY_ROW_SYS_RUN_TIME1 3 // row 3
#define DISPLAY_ROW_SEPARATOR 5     // row 5
#define DISPLAY_ROW_ALARM_STATUS0 6 // row 6
#define DISPLAY_ROW_ALARM_STATUS1 7 // row 7

static void uiIpAddress(uint8_t ip[4])
{
    DBGLOG(Debug, "IP: %d.%d.%d.%d => turn LED on", ip[0], ip[1], ip[2], ip[3]);

    char *ptr = dpCharBuf[DISPLAY_ROW_APP_STATUS0];
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_APP_STATUS0]), "IP address:     ");
    ssd1306_oled_set_XY(0, DISPLAY_ROW_APP_STATUS0);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);

    // max length of string is " 192.168.100.127"
    ptr = dpCharBuf[DISPLAY_ROW_APP_STATUS1];
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_APP_STATUS1]), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    ssd1306_oled_set_XY(0, DISPLAY_ROW_APP_STATUS1);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);
}

static void uiRunTime(uint32_t time)
{
    uint16_t s = time % 60;
    time /= 60;
    uint16_t m = time % 60;
    time /= 60;
    uint16_t h = time % 24;
    time /= 24;
    uint16_t d = time % 365;

    char *ptr = dpCharBuf[DISPLAY_ROW_SYS_RUN_TIME0];
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_SYS_RUN_TIME0]), "Run time:       ");
    ssd1306_oled_set_XY(0, DISPLAY_ROW_SYS_RUN_TIME0);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);

    ptr = dpCharBuf[DISPLAY_ROW_SYS_RUN_TIME1];
    memset(ptr, 0, NUM_CHAR_PER_COL);
    // max length of string is "365d 23h 59m 59s"
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_SYS_RUN_TIME1]), "%3dd %02dh %02dm %02ds", d, h, m, s);
    ssd1306_oled_set_XY(0, DISPLAY_ROW_SYS_RUN_TIME1);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);
}

static void uiAlarmStatus(bool alarmOn)
{
    char *ptr = dpCharBuf[DISPLAY_ROW_SEPARATOR];
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_SEPARATOR]), "================");
    ssd1306_oled_set_XY(0, DISPLAY_ROW_SEPARATOR);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);

    ptr = dpCharBuf[DISPLAY_ROW_ALARM_STATUS0];
    snprintf(ptr, sizeof(dpCharBuf[DISPLAY_ROW_ALARM_STATUS0]), "Status:         ");
    ssd1306_oled_set_XY(0, DISPLAY_ROW_ALARM_STATUS0);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, ptr);

    ssd1306_oled_set_XY(0, DISPLAY_ROW_ALARM_STATUS1);
    ssd1306_oled_write_line(SSD1306_FONT_NORMAL, alarmOn ? "        Alarm ON" : "       alarm OFF");
}

///////////////////////////////////////////////////////////////////////////////
unsigned int gSysEvent = 0;

///////////////////////////////////////////////////////////////////////////////
static unsigned int get_event(void)
{
    portDISABLE_INTERRUPTS();
    unsigned int ev = gSysEvent;
    gSysEvent = 0;
    portENABLE_INTERRUPTS();

    return ev;
}

static void set_event(unsigned int flags)
{
    portDISABLE_INTERRUPTS();
    gSysEvent |= flags;
    portENABLE_INTERRUPTS();
}

static __IO uint32_t TimingDelay;

void delay(__IO uint32_t milliseconds)
{
    TimingDelay = milliseconds;

    while (TimingDelay != 0)
    {
        __WFE();
    }
}

void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

static void DUALTIMER_Config(void)
{
    DUALTIMER_InitTypDef DUALTIMER_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // init timer0 as 1Hz
    DUALTIMER_InitStructure.Timer_Load = GetSystemClock() / 1; // 1s
    DUALTIMER_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    DUALTIMER_InitStructure.Timer_Wrapping = DUALTIMER_Periodic;
    DUALTIMER_InitStructure.Timer_Repetition = DUALTIMER_Wrapping;
    DUALTIMER_InitStructure.Timer_Size = DUALTIMER_Size_32;
    DUALTIMER_Init(DUALTIMER0_0, &DUALTIMER_InitStructure);

    // init timer1 as 2Hz
    DUALTIMER_InitStructure.Timer_Load = GetSystemClock() / 2; // 0.5s
    DUALTIMER_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    DUALTIMER_InitStructure.Timer_Wrapping = DUALTIMER_Periodic;
    DUALTIMER_InitStructure.Timer_Repetition = DUALTIMER_Wrapping;
    DUALTIMER_InitStructure.Timer_Size = DUALTIMER_Size_32;
    DUALTIMER_Init(DUALTIMER1_0, &DUALTIMER_InitStructure);

    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);
    DUALTIMER_ITConfig(DUALTIMER1_0, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DUALTIMER0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = DUALTIMER1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DUALTIMER_Cmd(DUALTIMER0_0, ENABLE);
    DUALTIMER_Cmd(DUALTIMER1_0, ENABLE);
}

static void UART_Config(void)
{
    UART_InitTypeDef UART_InitStructure;

    UART_StructInit(&UART_InitStructure);

#if defined(USE_WIZWIKI_W7500_EVAL)
    UART_Init(UART1, &UART_InitStructure);
    UART_Cmd(UART1, ENABLE);
#else
    S_UART_Init(115200);
    S_UART_Cmd(ENABLE);
#endif
}

static void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_ITInitTypeDef GPIO_ITInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_InitStructure.GPIO_Pin = USER_LED_PIN;
    GPIO_InitStructure.GPIO_Direction = GPIO_Direction_OUT;
    // GPIO_InitStructure.GPIO_Pad = GPIO_OpenDrainDisable | GPIO_HighDrivingStrength | GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_AF = PAD_AF1;
    GPIO_Init(USER_LED_PORT, &GPIO_InitStructure);

    USER_LED_OFF();

    GPIO_InitStructure.GPIO_Pin = NPU_INT_PIN;
    GPIO_InitStructure.GPIO_Direction = GPIO_Direction_IN;
    GPIO_InitStructure.GPIO_Pad = GPIO_InputBufferEnable | GPIO_CMOS | GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_AF = PAD_AF1;
    GPIO_Init(NPU_INT_PORT, &GPIO_InitStructure);

    GPIO_ITInitStructure.GPIO_IT_Pin = NPU_INT_PIN;
    GPIO_ITInitStructure.GPIO_IT_Polarity = GPIO_IT_HighRising;
    GPIO_ITInitStructure.GPIO_IT_Type = GPIO_IT_Edge;
    GPIO_IT_Init(NPU_INT_PORT, &GPIO_ITInitStructure);
    // GPIO_ITConfig(NPU_INT_PORT, NPU_INT_PIN, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = PORT2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void init_w7500(void)
{
    // Initialize PHY
#ifdef W7500
    DBGLOG(Debug, "PHY Init : %s", PHY_Init(GPIOB, GPIO_Pin_15, GPIO_Pin_14) == SET ? "Success" : "Fail");
#elif defined(W7500P)
    DBGLOG(Debug, "PHY Init : %s", PHY_Init(GPIOB, GPIO_Pin_14, GPIO_Pin_15) == SET ? "Success" : "Fail");
#endif

    DBGLOG(Debug, "Link : %s", PHY_GetLinkStatus() == PHY_LINK_ON ? "On" : "Off");

    memcpy(gWIZNETINFO.mac, MAC_ADDR, sizeof(MAC_ADDR));
    gWIZNETINFO.dhcp = NETINFO_DHCP;

    ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);
}

// User LED: on when IP is assigned
static void update_user_led(void)
{
    if (!is_ip_null(gWIZNETINFO.ip))
    {
        DBGLOG(Debug, "IP: %d.%d.%d.%d => turn LED on", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
        uiIpAddress(gWIZNETINFO.ip);
        USER_LED_ON();
    }
    else
    {
        DBGLOG(Debug, "is_ip_null(gWIZNETINFO.ip) return true => turn LED off");
        USER_LED_OFF();
    }
}

static void show_wiznetinfo(void)
{
    DBGLOG(Debug, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    DBGLOG(Debug, "IP: %d.%d.%d.%d", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    DBGLOG(Debug, "GW: %d.%d.%d.%d", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    DBGLOG(Debug, "SN: %d.%d.%d.%d", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    DBGLOG(Debug, "DNS: %d.%d.%d.%d", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
}

static void dhcp_assign(void)
{
    DBGLOG(Debug, "dhcp_assign()");

    getIPfromDHCP(gWIZNETINFO.ip);
    getGWfromDHCP(gWIZNETINFO.gw);
    getSNfromDHCP(gWIZNETINFO.sn);
    getDNSfromDHCP(gWIZNETINFO.dns);

    ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);

    update_user_led();
}

static void dhcp_update(void)
{
    DBGLOG(Debug, "dhcp_update()");
}

static void dhcp_conflict(void)
{
    DBGLOG(Debug, "dhcp_conflict()");
}

static void init_dhcp(void)
{
    if (gWIZNETINFO.dhcp == NETINFO_DHCP)
    {
        // DHCP Process
        DHCP_init(SOCKET_NUM_DHCP, (uint8_t *)get_share_buf());
        reg_dhcp_cbfunc(dhcp_assign, dhcp_update, dhcp_conflict);
    }
}

static void task_dhcp(void)
{
    static uint8_t dhcp_retry = 0;

    // DBGLOG(Debug, "calling DHCP_run(): dhcp_retry=%d", dhcp_retry);
    uint32_t ret = DHCP_run();
    switch (ret)
    {
    case DHCP_RUNNING:
        break;

    case DHCP_IP_LEASED:
        is_ip_assigned = true;
        show_wiznetinfo();
        break;

    case DHCP_FAILED:
        dhcp_retry++;
        DBGLOG(Debug, "DHCP_run() returns DHCP_FAILED: dhcp_retry=%d", dhcp_retry);
        break;

    default:
        DBGLOG(Debug, "Unsupported DHCP_run() return: %ld, dhcp_retry=%d", ret, dhcp_retry);
        break;
    }
}

static void init_dns(void)
{
    DNS_init(SOCKET_NUM_DNS, (uint8_t *)get_share_buf());
}

static void task_dns(void)
{
    static uint8_t dns_retry = 0;
    uint8_t dns_domain_ip[4];

    uint8_t *dns_domain_name = (uint8_t *)get_host_name();
    // DBGLOG(Debug, "calling DNS_run(): dns_retry=%d", dns_retry);
    uint32_t ret = DNS_run(gWIZNETINFO.dns, dns_domain_name, dns_domain_ip);
    if (ret == 1)
    {
        is_dns_ip_found = true;
        set_host_ip(dns_domain_ip);
        DBGLOG(Debug, "[%s] of ip : %d.%d.%d.%d", dns_domain_name, dns_domain_ip[0], dns_domain_ip[1], dns_domain_ip[2], dns_domain_ip[3]);
    }
    else
    {
        dns_retry++;
    }
}

static void messaging_callback(const MessageStatus status)
{
    switch (status)
    {
    case MessageConnecting:
        DBGLOG(Debug, "MessageConnecting");
        break;

    case MessageSending:
        DBGLOG(Debug, "MessageSending");
        break;

    case MessageSentSuccess:
        DBGLOG(Debug, "MessageSentSuccess");
        break;

    case MessageSentFail:
        DBGLOG(Debug, "MessageSentFail");
        break;

    default:
        DBGLOG(Debug, "unsupported status=%d", status);
        break;
    }
}

static bool init_lcd(void)
{
    if (!ssd1306_init(SSD1306_I2C_ADDR))
    {
        DBGLOG(Error, "ssd1306_init(0x%02x) FAILED", SSD1306_I2C_ADDR);
        return false;
    }

    if (!ssd1306_oled_default_config(LCD_ROW, LCD_COL))
    {
        DBGLOG(Error, "ssd1306_oled_default_config(%d, %d) FAILED", LCD_ROW, LCD_COL);
        return false;
    }

    if (!ssd1306_oled_clear_screen())
    {
        DBGLOG(Error, "ssd1306_oled_clear_screen() FAILED");
        return false;
    }

    if (!ssd1306_oled_onoff(true))
    {
        DBGLOG(Error, "ssd1306_oled_onoff(true) FAILED");
        return false;
    }

    return ssd1306_oled_write_line(SSD1306_FONT_NORMAL, "initializing ...");
}

#define BOOL2STR(b) (b ? "true" : "false")

static void init_peripheral(void)
{
    SystemInit();
    SysTick_Config((GetSystemClock() / 1000));
    setTIC100US((GetSystemClock() / 10000));
    UART_Config();
    GPIO_Config();
    DUALTIMER_Config();

    DBGLOG(Debug, "SourceClock : %d", (int)GetSourceClock());
    DBGLOG(Debug, "SystemClock : %d", (int)GetSystemClock());

    bool rst = i2c_init();
    DBGLOG(Debug, "i2c_init() returns %s", BOOL2STR(rst));

    rst = init_lcd();
    DBGLOG(Debug, "init_lcd() returns %s", BOOL2STR(rst));
}

int main(void)
{
    init_peripheral();
    init_w7500();

    show_wiznetinfo();
    init_dhcp();
    init_dns();
    set_messaging_callback(messaging_callback);
    task_npu_init();

    DBGLOG(Debug, "start eventLoop");
    int count_1hz = 0;
    int count_2hz = 0;
    while (1)
    {
        unsigned int ev = get_event();
        if (ev)
        {
            if (ev & EV_GPIO_NPU)
            {
                static bool mockIsAlarmOn = false;
                mockIsAlarmOn = !mockIsAlarmOn;
                DBGLOG(Debug, "EV_GPIO_NPU: mockIsAlarmOn=%d", mockIsAlarmOn);
                send_whatsapp(mockIsAlarmOn ? "Alert: alarm ON" : "alarm off");
                // send_whatsapp("w7500x");
                uiAlarmStatus(mockIsAlarmOn);
            }
            if (ev & EV_TIMER_2HZ)
            {
                // DBGLOG(Debug, "EV_TIMER_2HZ:count_2hz=%d", count_2hz);
                unsigned int rst = task_npu(EV_TIMER_2HZ);
                if (rst & EV_ALARM_ON)
                {
                    DBGLOG(Debug, "EV_ALARM_ON");
                    send_whatsapp("Alert: alarm ON");
                    uiAlarmStatus(true);
                }
                else if (rst & EV_ALARM_OFF)
                {
                    DBGLOG(Debug, "EV_ALARM_OFF");
                    send_whatsapp("alarm off");
                    uiAlarmStatus(false);
                }
                count_2hz++;
            }
            if (ev & EV_TIMER_1HZ)
            {
                // DBGLOG(Debug, "EV_TIMER_1HZ:count_1hz=%d\r\n", count_1hz);
                if (gWIZNETINFO.dhcp == NETINFO_DHCP && !is_ip_assigned)
                {
                    task_dhcp();
                }
                if (is_ip_assigned)
                {
                    if (!is_dns_ip_found)
                    {
                        task_dns();
                    }
                    else
                    {
                        task_messaging();
                    }
                }
                unsigned int rst = task_npu(EV_TIMER_1HZ);
                if (rst)
                {
                    set_event(rst);
                }

                uiRunTime(gSysRunTime);
                // uiRunTime(count_1hz);
                count_1hz++;
                gSysRunTime++;
            }
        }
        else
        {
            __WFE();
        }
    }

    return 0;
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
        portDISABLE_INTERRUPTS();
        __WFE();
    }
}
#endif
