/*
 * QSPI Flash driver for Adafruit nRF52 Bootloader
 * 
 * Copyright (c) 2024
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

#include "qspi_flash.h"
#include "nrfx_qspi.h"
#include "nrf_gpio.h"
#include <string.h>

// QSPI Flash configuration
static qspi_flash_config_t g_qspi_config = {0};
static bool g_qspi_initialized = false;

// Default QSPI configuration for W25Q16
static const nrfx_qspi_config_t qspi_config_default = {
    .xip_offset = 0x100000,  // Start after 1MB internal flash
    .pins = {
        .sck_pin  = 13,
        .csn_pin  = 14,
        .io0_pin  = 15,
        .io1_pin  = 16,
        .io2_pin  = 17,
        .io3_pin  = 18,
    },
    .prot_if = {
        .readoc    = NRF_QSPI_READOC_FASTREAD,
        .writeoc   = NRF_QSPI_WRITEOC_PP,
        .addrmode  = NRF_QSPI_ADDRMODE_24BIT,
        .dpmconfig = false,
    },
    .phy_if = {
        .sck_freq  = NRF_QSPI_FREQ_32MDIV8,  // 4MHz
        .sck_delay = 5,
        .dpmen     = false,
        .spi_mode  = NRF_QSPI_MODE_0,
    },
    .irq_priority = 7,
};

// W25Q16 commands
static const nrf_qspi_cinstr_conf_t cmd_write_enable = {
    .opcode    = W25Q16_CMD_WRITE_ENABLE,
    .length    = NRF_QSPI_CINSTR_LEN_1B,
    .io2_level = false,
    .io3_level = false,
    .wipwait   = false,
    .wren      = false,
};

static const nrf_qspi_cinstr_conf_t cmd_read_status = {
    .opcode    = W25Q16_CMD_READ_STATUS_REG1,
    .length    = NRF_QSPI_CINSTR_LEN_2B,
    .io2_level = false,
    .io3_level = false,
    .wipwait   = false,
    .wren      = false,
};

// Initialize QSPI Flash
qspi_flash_status_t qspi_flash_init(void)
{
    if (g_qspi_initialized) {
        return QSPI_FLASH_STATUS_SUCCESS;
    }

    // Configure pins from board.mk if available
#ifdef QSPI_SCK_PIN
    g_qspi_config.pins.sck_pin = QSPI_SCK_PIN;
#else
    g_qspi_config.pins.sck_pin = qspi_config_default.pins.sck_pin;
#endif

#ifdef QSPI_CSN_PIN
    g_qspi_config.pins.csn_pin = QSPI_CSN_PIN;
#else
    g_qspi_config.pins.csn_pin = qspi_config_default.pins.csn_pin;
#endif

#ifdef QSPI_IO0_PIN
    g_qspi_config.pins.io0_pin = QSPI_IO0_PIN;
#else
    g_qspi_config.pins.io0_pin = qspi_config_default.pins.io0_pin;
#endif

#ifdef QSPI_IO1_PIN
    g_qspi_config.pins.io1_pin = QSPI_IO1_PIN;
#else
    g_qspi_config.pins.io1_pin = qspi_config_default.pins.io1_pin;
#endif

#ifdef QSPI_IO2_PIN
    g_qspi_config.pins.io2_pin = QSPI_IO2_PIN;
#else
    g_qspi_config.pins.io2_pin = qspi_config_default.pins.io2_pin;
#endif

#ifdef QSPI_IO3_PIN
    g_qspi_config.pins.io3_pin = QSPI_IO3_PIN;
#else
    g_qspi_config.pins.io3_pin = qspi_config_default.pins.io3_pin;
#endif

    // Configure protocol interface
    g_qspi_config.prot_if.readoc    = qspi_config_default.prot_if.readoc;
    g_qspi_config.prot_if.writeoc   = qspi_config_default.prot_if.writeoc;
    g_qspi_config.prot_if.addrmode  = qspi_config_default.prot_if.addrmode;
    g_qspi_config.prot_if.dpmconfig = qspi_config_default.prot_if.dpmconfig;

    // Configure physical interface
    g_qspi_config.phy_if.sck_freq  = qspi_config_default.phy_if.sck_freq;
    g_qspi_config.phy_if.sck_delay = qspi_config_default.phy_if.sck_delay;
    g_qspi_config.phy_if.dpmen     = qspi_config_default.phy_if.dpmen;
    g_qspi_config.phy_if.spi_mode  = qspi_config_default.phy_if.spi_mode;

    // Set flash size and XIP offset
#ifdef QSPI_FLASH_SIZE
    g_qspi_config.flash_size = QSPI_FLASH_SIZE;
#else
    g_qspi_config.flash_size = W25Q16_TOTAL_SIZE;
#endif

#ifdef QSPI_XIP_OFFSET
    g_qspi_config.xip_offset = QSPI_XIP_OFFSET;
#else
    g_qspi_config.xip_offset = qspi_config_default.xip_offset;
#endif

    // Initialize QSPI driver
    nrfx_err_t err = nrfx_qspi_init(&qspi_config_default, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Set XIP offset
    nrf_qspi_xip_offset_set(NRF_QSPI, g_qspi_config.xip_offset);

    // Wait for QSPI to be ready
    if (qspi_flash_is_busy()) {
        return QSPI_FLASH_STATUS_BUSY;
    }

    g_qspi_initialized = true;
    return QSPI_FLASH_STATUS_SUCCESS;
}

// Deinitialize QSPI Flash
void qspi_flash_deinit(void)
{
    if (g_qspi_initialized) {
        nrfx_qspi_uninit();
        g_qspi_initialized = false;
    }
}

// Check if QSPI Flash is busy
bool qspi_flash_is_busy(void)
{
    uint8_t status = qspi_flash_get_status();
    return (status & W25Q16_STATUS_BUSY) != 0;
}

// Get QSPI Flash status register
uint8_t qspi_flash_get_status(void)
{
    uint8_t tx_data = 0;
    uint8_t rx_data = 0;
    
    nrfx_qspi_cinstr_xfer(&cmd_read_status, &tx_data, &rx_data);
    return rx_data;
}

// Wait for QSPI Flash to be ready
static qspi_flash_status_t qspi_flash_wait_ready(uint32_t timeout_ms)
{
    uint32_t start_time = 0; // TODO: implement proper timing
    
    while (qspi_flash_is_busy()) {
        // TODO: add timeout check
        if (timeout_ms > 0) {
            // Simple delay
            for (volatile uint32_t i = 0; i < 1000; i++);
            timeout_ms--;
            if (timeout_ms == 0) {
                return QSPI_FLASH_STATUS_TIMEOUT;
            }
        }
    }
    
    return QSPI_FLASH_STATUS_SUCCESS;
}

// Enable write operations
static qspi_flash_status_t qspi_flash_write_enable(void)
{
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(&cmd_write_enable, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }
    
    return QSPI_FLASH_STATUS_SUCCESS;
}

// Read data from QSPI Flash
qspi_flash_status_t qspi_flash_read(uint32_t address, uint8_t *data, size_t length)
{
    if (!g_qspi_initialized || !data || length == 0) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Check address bounds
    if (address >= g_qspi_config.flash_size) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Limit read to flash size
    if (address + length > g_qspi_config.flash_size) {
        length = g_qspi_config.flash_size - address;
    }

    nrfx_err_t err = nrfx_qspi_read(data, length, address);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    return QSPI_FLASH_STATUS_SUCCESS;
}

// Write data to QSPI Flash
qspi_flash_status_t qspi_flash_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (!g_qspi_initialized || !data || length == 0) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Check address bounds
    if (address >= g_qspi_config.flash_size) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Limit write to flash size
    if (address + length > g_qspi_config.flash_size) {
        length = g_qspi_config.flash_size - address;
    }

    // Wait for previous operations to complete
    qspi_flash_status_t status = qspi_flash_wait_ready(1000);
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Enable write
    status = qspi_flash_write_enable();
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Write data
    nrfx_err_t err = nrfx_qspi_write(data, length, address);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Wait for write to complete
    return qspi_flash_wait_ready(5000); // 5 second timeout for write
}

// Erase sector in QSPI Flash
qspi_flash_status_t qspi_flash_erase_sector(uint32_t address)
{
    if (!g_qspi_initialized) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Align address to sector boundary
    address = (address / W25Q16_SECTOR_SIZE) * W25Q16_SECTOR_SIZE;

    // Check address bounds
    if (address >= g_qspi_config.flash_size) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Wait for previous operations to complete
    qspi_flash_status_t status = qspi_flash_wait_ready(1000);
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Enable write
    status = qspi_flash_write_enable();
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Erase sector
    nrfx_err_t err = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, address);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Wait for erase to complete
    return qspi_flash_wait_ready(5000); // 5 second timeout for erase
}

// Erase block in QSPI Flash
qspi_flash_status_t qspi_flash_erase_block(uint32_t address, size_t size)
{
    if (!g_qspi_initialized) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Check address bounds
    if (address >= g_qspi_config.flash_size) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    qspi_flash_status_t status = QSPI_FLASH_STATUS_SUCCESS;

    // Erase in 64KB blocks
    while (size > 0) {
        size_t block_size = (size >= W25Q16_BLOCK_SIZE_64KB) ? W25Q16_BLOCK_SIZE_64KB : size;
        
        // Wait for previous operations to complete
        status = qspi_flash_wait_ready(1000);
        if (status != QSPI_FLASH_STATUS_SUCCESS) {
            return status;
        }

        // Enable write
        status = qspi_flash_write_enable();
        if (status != QSPI_FLASH_STATUS_SUCCESS) {
            return status;
        }

        // Erase block
        nrfx_err_t err = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, address);
        if (err != NRFX_SUCCESS) {
            return QSPI_FLASH_STATUS_ERROR;
        }

        // Wait for erase to complete
        status = qspi_flash_wait_ready(10000); // 10 second timeout for block erase
        if (status != QSPI_FLASH_STATUS_SUCCESS) {
            return status;
        }

        address += block_size;
        size -= block_size;
    }

    return QSPI_FLASH_STATUS_SUCCESS;
}

// Erase entire QSPI Flash
qspi_flash_status_t qspi_flash_chip_erase(void)
{
    if (!g_qspi_initialized) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Wait for previous operations to complete
    qspi_flash_status_t status = qspi_flash_wait_ready(1000);
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Enable write
    status = qspi_flash_write_enable();
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }

    // Chip erase
    nrfx_err_t err = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_ALL, 0);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Wait for erase to complete
    return qspi_flash_wait_ready(60000); // 60 second timeout for chip erase
}

// Set QSPI Flash XIPOFFSET
void qspi_flash_set_xip_offset(uint32_t offset)
{
    if (g_qspi_initialized) {
        nrf_qspi_xip_offset_set(NRF_QSPI, offset);
        g_qspi_config.xip_offset = offset;
    }
}

// Get QSPI Flash configuration
const qspi_flash_config_t* qspi_flash_get_config(void)
{
    return &g_qspi_config;
}