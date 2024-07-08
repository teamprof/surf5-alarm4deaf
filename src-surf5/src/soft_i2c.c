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
#include <stdio.h>
#include "soft_i2c.h"
#include "pins.h"
#include "./AppLog.h"

///////////////////////////////////////////////////////////////////////////////
#define I2C_SCL_PIN MCU_SCL_PIN
#define I2C_SCL_PPORT MCU_SCL_PORT
#define I2C_SDA_PIN MCU_SDA_PIN
#define I2C_SDA_PORT MCU_SDA_PORT
#define I2C_DELAY 5 // 100kHz => 10us = 2 * 5us

#define I2C_WRITE 0
#define I2C_READ 1

///////////////////////////////////////////////////////////////////////////////
static inline void i2c_scl_high(void)
{
    I2C_SCL_PPORT->OUTENCLR = I2C_SCL_PIN;
}
static inline void i2c_scl_low(void)
{
    I2C_SCL_PPORT->OUTENSET = I2C_SCL_PIN;
}
static inline bool i2c_scl_read(void)
{
    return ((I2C_SCL_PPORT->DATA & I2C_SCL_PIN) != (uint32_t)Bit_RESET);
}

static inline void i2c_sda_high(void)
{
    I2C_SDA_PORT->OUTENCLR = I2C_SDA_PIN;
}
static inline void i2c_sda_low(void)
{
    I2C_SDA_PORT->OUTENSET = I2C_SDA_PIN;
}
static inline bool i2c_sda_read(void)
{
    return ((I2C_SDA_PORT->DATA & I2C_SDA_PIN) != (uint32_t)Bit_RESET);
}

static void delay_us(volatile uint32_t us)
{
    while (us-- > 0)
    {
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

static inline void i2c_start(void)
{
    // assumption: scl=high, sda=high at this point
    i2c_sda_low();
    delay_us(I2C_DELAY);
    i2c_scl_low();
}

static inline void i2c_stop(void)
{
    // assumption: scl=low, sda=* at this point
    i2c_sda_low();
    delay_us(I2C_DELAY);
    i2c_scl_high();
    delay_us(I2C_DELAY);
    i2c_sda_high();
}

bool i2c_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // GPIO_ITInitTypeDef GPIO_ITInitStructure;

    // SCL setting
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN;
    // GPIO_InitStructure.GPIO_Direction = GPIO_Direction_OUT;
    GPIO_InitStructure.GPIO_Direction = GPIO_Direction_IN;
    GPIO_InitStructure.GPIO_Pad = GPIO_OpenDrainEnable | GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_AF = PAD_AF1;
    GPIO_Init(I2C_SCL_PPORT, &GPIO_InitStructure);
    GPIO_ResetBits(I2C_SCL_PPORT, I2C_SCL_PIN);

    // SDA set ting
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    // GPIO_InitStructure.GPIO_Direction = GPIO_Direction_OUT;
    GPIO_InitStructure.GPIO_Direction = GPIO_Direction_IN;
    GPIO_InitStructure.GPIO_Pad = GPIO_OpenDrainEnable | GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_AF = PAD_AF1;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(I2C_SDA_PORT, I2C_SDA_PIN);

    return true;
}

static inline bool i2c_read_bit(void)
{
    i2c_sda_high();
    delay_us(I2C_DELAY);
    i2c_scl_high();

    bool bitScl;
    for (int i = 0; i < 100; i++)
    {
        bitScl = i2c_scl_read();
        if (bitScl)
        {
            break;
        }
    }

    delay_us(I2C_DELAY);
    bool bitSda = bitScl ? i2c_sda_read() : true;
    i2c_scl_low();
    return bitSda;
}

static inline bool i2c_byte_out(uint8_t b)
{
    // assumption: scl=low, sda=low at this point
    // DBGLOG(Debug, "0x%02x", b);
    for (int i = 0; i < 8; i++)
    {
        if (b & 0x80)
            i2c_sda_high();
        else
            i2c_sda_low();

        i2c_scl_high();
        delay_us(I2C_DELAY);
        i2c_scl_low();
        delay_us(I2C_DELAY);
        b <<= 1;
    }
    bool nack = i2c_read_bit();
    return !nack;
}

static inline uint8_t i2c_byte_in(uint8_t bLast)
{
    uint8_t b = 0;

    i2c_sda_high();
    for (int i = 0; i < 8; i++)
    {
        b = (b << 1) | i2c_read_bit();
    }

    if (bLast)
        i2c_sda_high(); // last byte sends a NACK
    else
        i2c_sda_low();
    i2c_scl_high();
    delay_us(I2C_DELAY);
    i2c_scl_low(); // clock low to send ack
    delay_us(I2C_DELAY);
    i2c_sda_low();
    return b;
}

static inline bool i2c_begin_transmission(uint8_t addr, uint8_t bRead)
{
    i2c_start();
    addr <<= 1;
    if (bRead)
        addr++;                // set read bit
    return i2c_byte_out(addr); // send the slave address and R/W bit
}

static inline void i2c_end_transmission(void)
{
    i2c_stop();
}

static int i2c_read_bytes(uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        *data++ = i2c_byte_in(i >= (len - 1));
    }
    return i;
}

bool i2c_scan(uint8_t reg)
{
    bool ret = i2c_begin_transmission(reg, 0);
    i2c_end_transmission();
    return ret;
}

int i2c_read_register(uint8_t addr, uint8_t reg, uint8_t *data, int len)
{
    // DBGLOG(Debug, "addr=0x%02x, reg=0x%02x, len=%d", addr, reg, len);
    int rdSize = 0;
    bool rc = i2c_begin_transmission(addr, I2C_WRITE); // start a write operation
    if (rc)                                            // slave sent ACK for its address
    {
        delay_us(I2C_DELAY);
        rc = i2c_byte_out(reg); // write the register we want to read from
        if (rc)
        {
            delay_us(I2C_DELAY);
            i2c_end_transmission();
            delay_us(I2C_DELAY);
            rc = i2c_begin_transmission(addr, I2C_READ); // start a read operation
            if (rc)
            {
                delay_us(I2C_DELAY);
                rdSize = i2c_read_bytes(data, len);
            }
        }
    }
    delay_us(I2C_DELAY);
    i2c_end_transmission();
    return rdSize;
}
int i2c_write_register(uint8_t addr, uint8_t reg, const uint8_t *data, int len)
{
    // DBGLOG(Debug, "addr=0x%02x, reg=0x%02x, len=%d", addr, reg, len);
    int wrSize = 0;
    bool rc = i2c_begin_transmission(addr, I2C_WRITE); // start a write operation
    if (rc)                                            // slave sent ACK for its address
    {
        rc = i2c_byte_out(reg); // write the register we want to read from
        while (rc && (len-- > 0))
        {
            rc = i2c_byte_out(*data++);
            wrSize++;
        }
    }
    i2c_end_transmission();
    return wrSize;
}
