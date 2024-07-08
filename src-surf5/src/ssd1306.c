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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "soft_i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "./AppLog.h"

typedef struct _lcd_dev
{
    uint8_t i2c_addr;
    uint8_t max_lines;
    uint8_t max_columns;
    uint8_t global_x;
    uint8_t global_y;
    uint8_t buf[1024];
} lcd_dev;

static lcd_dev _lcd = {0};

bool ssd1306_init(uint8_t i2c_dev)
{
    if (_lcd.i2c_addr)
    {
        return false;
    }
    _lcd.i2c_addr = i2c_dev;
    return true;
}

bool ssd1306_end(void)
{
    memset(&_lcd, 0, sizeof(_lcd));
    return true;
}

bool ssd1306_oled_default_config(uint8_t oled_lines, uint8_t oled_columns)
{
    if (oled_lines != SSD1306_128_64_LINES && oled_lines != SSD1306_128_32_LINES && SSD1306_64_48_LINES)
        oled_lines = SSD1306_128_64_LINES;

    if (oled_columns != SSD1306_128_64_COLUMNS && oled_lines != SSD1306_128_32_COLUMNS && SSD1306_64_48_COLUMNS)
        oled_columns = SSD1306_128_64_COLUMNS;

    _lcd.max_lines = oled_lines;
    _lcd.max_columns = oled_columns;
    _lcd.global_x = 0;
    _lcd.global_y = 0;

    uint8_t *data_buf = _lcd.buf;
    int size = 0;
    data_buf[size++] = SSD1306_COMM_DISPLAY_OFF; // display off
    data_buf[size++] = SSD1306_COMM_DISP_NORM;   // Set Normal Display (default)
    data_buf[size++] = SSD1306_COMM_CLK_SET;     // SETDISPLAYCLOCKDIV
    data_buf[size++] = 0x80;                     // the suggested ratio 0x80
    data_buf[size++] = SSD1306_COMM_MULTIPLEX;   // SSD1306_SETMULTIPLEX
    data_buf[size++] = oled_lines - 1;           // height is 32 or 64 (always -1)
    data_buf[size++] = SSD1306_COMM_VERT_OFFSET; // SETDISPLAYOFFSET
    data_buf[size++] = 0;                        // no offset
    data_buf[size++] = SSD1306_COMM_START_LINE;  // SETSTARTLINE
    data_buf[size++] = SSD1306_COMM_CHARGE_PUMP; // CHARGEPUMP
    data_buf[size++] = 0x14;                     // turn on charge pump
    data_buf[size++] = SSD1306_COMM_MEMORY_MODE; // MEMORYMODE
    data_buf[size++] = SSD1306_PAGE_MODE;        // page mode
    data_buf[size++] = SSD1306_COMM_HORIZ_FLIP;  // SEGREMAP  Mirror screen vertically (A1)
    // data_buf[size++] = SSD1306_COMM_HORIZ_NORM;  // SEGREMAP  Mirror screen horizontally (A0)
    data_buf[size++] = SSD1306_COMM_SCAN_REVS; // COMSCANDEC Rotate screen vertically (C8)
    // data_buf[size++] = SSD1306_COMM_SCAN_NORM; // COMSCANDEC Rotate screen vertically (C0)
    data_buf[size++] = SSD1306_COMM_COM_PIN; // HARDWARE PIN
    if (oled_lines == 32)
        data_buf[size++] = 0x02; // for 32 lines
    else
        data_buf[size++] = 0x12;                    // for 64 lines or 48 lines
    data_buf[size++] = SSD1306_COMM_CONTRAST;       // SETCONTRAST
    data_buf[size++] = 0x7f;                        // default contract value
    data_buf[size++] = SSD1306_COMM_PRECHARGE;      // SETPRECHARGE
    data_buf[size++] = 0xf1;                        // default precharge value
    data_buf[size++] = SSD1306_COMM_DESELECT_LV;    // SETVCOMDETECT
    data_buf[size++] = 0x40;                        // default deselect value
    data_buf[size++] = SSD1306_COMM_RESUME_RAM;     // DISPLAYALLON_RESUME
    data_buf[size++] = SSD1306_COMM_DISP_NORM;      // NORMALDISPLAY
    data_buf[size++] = SSD1306_COMM_DISPLAY_ON;     // DISPLAY ON
    data_buf[size++] = SSD1306_COMM_DISABLE_SCROLL; // Stop scroll

    int rst = i2c_write_register(_lcd.i2c_addr, SSD1306_COMM_CONTROL_BYTE, data_buf, size);
    // DBGLOG(Debug, "i2c_write_register(0x%02x, 0x%02x, %p, %d) returns %d", _lcd.i2c_addr, SSD1306_COMM_CONTROL_BYTE, data_buf, size, rst);
    return (rst == size);
    // return i2c_write_register(_lcd.i2c_addr, SSD1306_COMM_CONTROL_BYTE, data_buf, size) == size;
}

bool ssd1306_oled_clear_line(uint8_t row)
{
    if (row >= (_lcd.max_lines / 8))
    {
        return false;
    }

    ssd1306_oled_set_XY(0, row);
    memset(_lcd.buf, 0, _lcd.max_columns);
    int size = _lcd.max_columns + 1;
    return i2c_write_register(_lcd.i2c_addr, SSD1306_DATA_CONTROL_BYTE, _lcd.buf, size) == size;
}

bool ssd1306_oled_clear_screen(void)
{
    for (uint8_t i = 0; i < (_lcd.max_lines / 8); i++)
    {
        if (!ssd1306_oled_clear_line(i))
        {
            return false;
        }
    }
    return true;
}

bool ssd1306_oled_onoff(bool onoff)
{
    uint8_t data = onoff ? SSD1306_COMM_DISPLAY_ON : SSD1306_COMM_DISPLAY_OFF;
    return i2c_write_register(_lcd.i2c_addr, SSD1306_COMM_CONTROL_BYTE, &data, sizeof(data)) == sizeof(data);
}

bool ssd1306_oled_set_XY(uint8_t x, uint8_t y)
{
    if (x >= _lcd.max_columns || y >= (_lcd.max_lines / 8))
    {
        return false;
    }

    _lcd.global_x = x;
    _lcd.global_y = y;

    uint8_t *data_buf = _lcd.buf;
    int size = 0;
    data_buf[size++] = SSD1306_COMM_PAGE_NUMBER | (y & 0x0f);
    data_buf[size++] = SSD1306_COMM_LOW_COLUMN | (x & 0x0f);
    data_buf[size++] = SSD1306_COMM_HIGH_COLUMN | ((x >> 4) & 0x0f);
    return i2c_write_register(_lcd.i2c_addr, SSD1306_COMM_CONTROL_BYTE, _lcd.buf, size) == size;
}

bool ssd1306_oled_write_line(uint8_t font_size, const char *data)
{
    uint16_t index = 0;
    uint8_t *font_table = 0;
    uint8_t font_table_width = 0;

    if (!data)
    {
        return false;
    }

    if (font_size == SSD1306_FONT_SMALL) // 5x7
    {
        font_table = (uint8_t *)font5x7;
        font_table_width = 5;
    }
    else if (font_size == SSD1306_FONT_NORMAL) // 8x8
    {
        font_table = (uint8_t *)font8x8;
        font_table_width = 8;
    }
    else
    {
        return false;
    }

    uint8_t *data_buf = _lcd.buf;
    int size = 0;

    // font table range in ascii table is from 0x20(space) to 0x7e(~)
    while (data[index] && size <= sizeof(_lcd.buf))
    {
        if ((data[index] < ' ') || (data[index] > '~'))
        {
            return false;
        }

        uint8_t *font_ptr = &font_table[(data[index] - 0x20) * font_table_width];
        for (uint8_t j = 0; j < font_table_width; j++)
        {
            data_buf[size++] = font_ptr[j];
            if (size >= sizeof(_lcd.buf))
            {
                return false;
            }
        }
        // insert 1 col space for small font size)
        if (font_size == SSD1306_FONT_SMALL)
        {
            data_buf[size++] = 0x00;
        }
        index++;
    }

    return i2c_write_register(_lcd.i2c_addr, SSD1306_DATA_CONTROL_BYTE, _lcd.buf, size) == size;
}
