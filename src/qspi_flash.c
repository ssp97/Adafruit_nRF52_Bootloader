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

#include "boards.h"
#include "qspi_flash.h"
#include "nrfx_qspi.h"
#include "nrf_gpio.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// QSPI Flash configuration
static nrfx_qspi_config_t g_qspi_config = {0};
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
        .readoc    = NRF_QSPI_READOC_READ4IO,  // Use Quad I/O for reading
        .writeoc   = NRF_QSPI_WRITEOC_PP4IO,   // Use Quad I/O for writing
        .addrmode  = NRF_QSPI_ADDRMODE_24BIT,
        .dpmconfig = true,
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

static const nrf_qspi_cinstr_conf_t cmd_write_status = {
    .opcode    = W25Q16_CMD_WRITE_STATUS_REG,
    .length    = NRF_QSPI_CINSTR_LEN_2B,
    .io2_level = false,
    .io3_level = false,
    .wipwait   = false,
    .wren      = true,
};

static const nrf_qspi_cinstr_conf_t cmd_read_status2 = {
    .opcode    = W25Q16_CMD_READ_STATUS_REG2,
    .length    = NRF_QSPI_CINSTR_LEN_2B,
    .io2_level = false,
    .io3_level = false,
    .wipwait   = false,
    .wren      = false,
};

static void qspi_wait_ready(void)
{
    uint16_t timeout = 1000;
    uint8_t sr = 0;
    do {
        sr = nrfx_qspi_mem_busy_check();
        timeout--;
    } while (sr && timeout);
}


// Initialize QSPI Flash
qspi_flash_status_t qspi_flash_init(void)
{
    if (g_qspi_initialized) {
        PRINTF("QSPI Flash already initialized\r\n");
        return QSPI_FLASH_STATUS_SUCCESS;
    }

    PRINTF("Initializing QSPI Flash...\r\n");

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


#ifdef QSPI_XIP_OFFSET
    g_qspi_config.xip_offset = QSPI_XIP_OFFSET;
#else
    g_qspi_config.xip_offset = qspi_config_default.xip_offset;
#endif

    // Initialize QSPI driver
    NRF_QSPI->DPMDUR = (0x3 << 16) | 0x3;
    PRINTF("QSPI pins: SCK=%d, CSN=%d, IO0=%d, IO1=%d, IO2=%d, IO3=%d\r\n",
            g_qspi_config.pins.sck_pin, g_qspi_config.pins.csn_pin,
            g_qspi_config.pins.io0_pin, g_qspi_config.pins.io1_pin,
            g_qspi_config.pins.io2_pin, g_qspi_config.pins.io3_pin);
    nrfx_err_t err = nrfx_qspi_init(&g_qspi_config, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        PRINTF("QSPI init failed: err=%d\r\n", err);
        return QSPI_FLASH_STATUS_ERROR;
    }
    PRINTF("QSPI driver initialized\r\n");

    // Set XIP offset
    nrf_qspi_xip_offset_set(NRF_QSPI, g_qspi_config.xip_offset);

    // Wait for QSPI to be ready
    qspi_wait_ready();
    
    // Configure W25Q16 for Quad mode
    PRINTF("Configuring W25Q16 for Quad mode...\r\n");
    qspi_flash_status_t quad_err = qspi_flash_configure_quad_mode();
    if (quad_err != QSPI_FLASH_STATUS_SUCCESS) {
        PRINTF("Failed to configure Quad mode: err=%d\r\n", quad_err);
        // Continue anyway, as the flash might still work in single mode
    }
    // PRINTF("QSPI Flash ready, size=%lu bytes, XIP offset=0x%08lX\r\n",
    //         g_qspi_config.flash_size, g_qspi_config.xip_offset);

    g_qspi_initialized = true;
    PRINTF("QSPI Flash initialization completed\r\n");
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
    // uint32_t start_time = 0; // TODO: implement proper timing
    
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

// Configure W25Q16 for Quad mode
static qspi_flash_status_t qspi_flash_configure_quad_mode(void)
{
    uint8_t status1, status2;
    uint8_t tx_data[2];
    
    // Read current status registers
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(&cmd_read_status, NULL, &status1);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }
    
    err = nrfx_qspi_cinstr_xfer(&cmd_read_status2, NULL, &status2);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }
    
    PRINTF("W25Q16 Status1: 0x%02X, Status2: 0x%02X\r\n", status1, status2);
    
    // Check if Quad mode is already enabled (QIO bit in Status Register 2)
    if ((status2 & 0x02) != 0) {
        PRINTF("W25Q16 already in Quad mode\r\n");
        return QSPI_FLASH_STATUS_SUCCESS;
    }
    
    // Enable Quad mode by setting QIO bit in Status Register 2
    // Status Register 2: bit 1 = Quad Enable (QIO)
    status2 |= 0x02;
    
    // Write enable before writing status register
    err = nrfx_qspi_cinstr_xfer(&cmd_write_enable, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }
    
    // Write status registers
    tx_data[0] = status1;
    tx_data[1] = status2;
    err = nrfx_qspi_cinstr_xfer(&cmd_write_status, tx_data, NULL);
    if (err != NRFX_SUCCESS) {
        return QSPI_FLASH_STATUS_ERROR;
    }
    
    // Wait for write to complete
    qspi_flash_status_t status = qspi_flash_wait_ready(1000);
    if (status != QSPI_FLASH_STATUS_SUCCESS) {
        return status;
    }
    
    PRINTF("W25Q16 configured for Quad mode\r\n");
    return QSPI_FLASH_STATUS_SUCCESS;
}

// Read data from QSPI Flash
qspi_flash_status_t qspi_flash_read(uint32_t address, uint8_t *data, size_t length)
{
    if (!g_qspi_initialized || !data || length == 0) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Check address bounds
    if (address >= QSPI_FLASH_SIZE) {
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Limit read to flash size
    if (address + length > QSPI_FLASH_SIZE) {
        length = QSPI_FLASH_SIZE - address;
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
        PRINTF("QSPI write error: not initialized or invalid params\r\n");
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Check address bounds
    if (address >= QSPI_FLASH_SIZE) {
        PRINTF("QSPI write error: address 0x%08lX out of bounds\r\n", address);
        return QSPI_FLASH_STATUS_ERROR;
    }

    // Limit write to flash size
    if (address + length > QSPI_FLASH_SIZE) {
        length = QSPI_FLASH_SIZE - address;
        PRINTF("QSPI write: limited length to %u bytes\r\n", length);
    }

    PRINTF("QSPI write: addr=0x%08lX, len=%u\r\n", address, length);

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
    if (address >= QSPI_FLASH_SIZE) {
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
    if (address >= QSPI_FLASH_SIZE) {
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
const nrfx_qspi_config_t* qspi_flash_get_config(void)
{
    return &g_qspi_config;
}