/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _FEATHER_NRF52840_H
#define _FEATHER_NRF52840_H

#define _PINNUM(port, pin)    ((port)*32 + (pin))

/*------------------------------------------------------------------*/
/* LED
 *------------------------------------------------------------------*/
#define LEDS_NUMBER           2
#define LED_PRIMARY_PIN       _PINNUM(1, 15)
#define LED_SECONDARY_PIN     _PINNUM(1, 10)
#define LED_STATE_ON          1

#define LED_NEOPIXEL          _PINNUM(0, 16)
#define NEOPIXEL_POWER_PIN    _PINNUM(1, 14)
#define NEOPIXELS_NUMBER      1
#define BOARD_RGB_BRIGHTNESS  0x040404

/*------------------------------------------------------------------*/
/* BUTTON
 *------------------------------------------------------------------*/
#define BUTTONS_NUMBER        2
#define BUTTON_1              _PINNUM(1, 02)
#define BUTTON_2              _PINNUM(0, 10)
#define BUTTON_PULL           NRF_GPIO_PIN_PULLUP

//--------------------------------------------------------------------+
// BLE OTA
//--------------------------------------------------------------------+
#define BLEDIS_MANUFACTURER   "gat-iot"
#define BLEDIS_MODEL          "gat562-mesh-watch"

//--------------------------------------------------------------------+
// USB
//--------------------------------------------------------------------+
#define USB_DESC_VID           0x239A
#define USB_DESC_UF2_PID       0x0029
#define USB_DESC_CDC_ONLY_PID  0x002A

//------------- UF2 -------------//
#define UF2_PRODUCT_NAME      "GAT562-MESH-WATCH"
#define UF2_VOLUME_LABEL      "GAT562"
#define UF2_BOARD_ID          "GAT562-MESH-WATCH"
#define UF2_INDEX_URL         "http://www.gat-iot.com/index.html"

// QSPI Flash Configuration
// Enable QSPI Flash support
#define ENABLE_QSPI_FLASH 1

// QSPI Flash size in bytes (W25Q16 = 2MB)
#define QSPI_FLASH_SIZE 2097152

// QSPI Flash XIPOFFSET (start address after internal flash)
#define QSPI_XIP_OFFSET 0x100000

// QSPI Flash pins configuration
#define QSPI_SCK_PIN 3
#define QSPI_CSN_PIN 26
#define QSPI_IO0_PIN 30
#define QSPI_IO1_PIN 29
#define QSPI_IO2_PIN 28
#define QSPI_IO3_PIN 2


#endif // _FEATHER_NRF52840_H
